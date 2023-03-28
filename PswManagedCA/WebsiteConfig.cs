using Microsoft.Deployment.WindowsInstaller;
using Microsoft.Web.Administration;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Xml.Serialization;
using PswManagedCA.Util;

namespace PswManagedCA
{
    public class WebsiteConfig
    {
        [Serializable]
        public class WebsiteConfigCatalog
        {
            public string Component { get; set; }
            public string Website { get; set; }
            public bool Stop { get; set; } = false;
            public bool Start { get; set; } = false;
            public bool? AutoStart { get; set; } = null;
            public ErrorHandling ErrorHandling { get; set; } = ErrorHandling.fail;
        }

        [CustomAction]
        public static ActionResult WebsiteConfigSched(Session session)
        {
            AssemblyName me = typeof(WebsiteConfig).Assembly.GetName();
            session.Log($"Initialized from {me.Name} v{me.Version}");

            List<WebsiteConfigCatalog> catalogs = new List<WebsiteConfigCatalog>();
            using (View view = session.Database.OpenView("SELECT `Component_`, `Website`, `Stop`, `Start`, `AutoStart`, `ErrorHandling` FROM `PSW_WebsiteConfig` ORDER BY `Order`"))
            {
                view.Execute(null);

                foreach (Record rec in view)
                {
                    WebsiteConfigCatalog cfg = new WebsiteConfigCatalog();
                    using (rec)
                    {
                        cfg.Component = rec.GetString("Component_");
                        cfg.Website = session.Format(rec.GetString("Website"));
                        cfg.Stop = (rec.GetInteger("Stop") != 0);
                        cfg.Start = (rec.GetInteger("Start") != 0);
                        int autoStart = rec.GetInteger("AutoStart");
                        cfg.AutoStart = (autoStart < 0) ? null : (autoStart == 0) ? (bool?)false : true;
                        cfg.ErrorHandling = (ErrorHandling)rec.GetInteger("ErrorHandling");
                    }

                    ComponentInfo ci = session.Components[cfg.Component];
                    if (ci == null)
                    {
                        session.Log($"Component '{cfg.Component}' not present in package");
                        return ActionResult.Failure;
                    }
                    switch (ci.RequestState)
                    {
                        case InstallState.Default:
                        case InstallState.Local:
                        case InstallState.Source:
                            break;

                        default:
                            session.Log($"Component '{ci.Name}' action isn't install, or repair. Skipping WebsiteConfig for '{cfg.Website}'");
                            continue;
                    }

                    if (string.IsNullOrEmpty(cfg.Website))
                    {
                        session.Log($"Website name is empty for component '{ci.Name}'");
                        return ActionResult.Failure;
                    }
                    session.Log($"Will configure website '{cfg.Website}'");
                    catalogs.Add(cfg);
                }
            }

            if (catalogs.Count > 0)
            {
                XmlSerializer srlz = new XmlSerializer(catalogs.GetType());
                using (StringWriter sw = new StringWriter())
                {
                    srlz.Serialize(sw, catalogs);
                    session["PSW_WebsiteConfigExec"] = sw.ToString();
                    session.DoAction("PSW_WebsiteConfigExec");
                }
            }

            return ActionResult.Success;
        }

        [CustomAction]
        public static ActionResult WebsiteConfigExec(Session session)
        {
            AssemblyName me = typeof(WebsiteConfig).Assembly.GetName();
            session.Log($"Initialized from {me.Name} v{me.Version}");

            List<WebsiteConfigCatalog> actions = new List<WebsiteConfigCatalog>();
            XmlSerializer srlz = new XmlSerializer(actions.GetType());
            string cad = session["CustomActionData"];
            using (StringReader sr = new StringReader(cad))
            {
                IEnumerable<WebsiteConfigCatalog> ctlgs = srlz.Deserialize(sr) as IEnumerable<WebsiteConfigCatalog>;
                if (ctlgs == null)
                {
                    return ActionResult.Success;
                }
                actions.AddRange(ctlgs);
            }

            foreach (WebsiteConfigCatalog ctlg in actions)
            {
            LRetry:
                try
                {
                    WebsiteConfigExec(session, ctlg);
                }
                catch (Exception ex)
                {
                    switch (session.HandleError(ctlg.ErrorHandling, 27007, ctlg.Website, ex.Message))
                    {
                        default: // Silent / fail
                            session.Log($"User aborted on failure to configure website {ctlg.Website}. {ex.Message}");
                            return ActionResult.Failure;

                        case MessageResult.Ignore:
                            session.Log($"User ignored failure to configure website {ctlg.Website}. {ex.Message}");
                            continue;

                        case MessageResult.Retry:
                            session.Log($"User retry on failure to configure website {ctlg.Website}. {ex.Message}");
                            goto LRetry;
                    }
                }
            }

            return ActionResult.Success;
        }

        private static void WebsiteConfigExec(Session session, WebsiteConfigCatalog cfg)
        {
            using (ServerManager manager = new ServerManager())
            {
                Site site = manager.Sites[cfg.Website];
                if (site == null)
                {
                    throw new Exception($"Could not find '{cfg.Website}' website");
                }

                if (cfg.Stop)
                {
                    session.Log($"Stopping site '{cfg.Website}'");
                    site.Stop();
                    manager.CommitChanges();

                    switch (site.State)
                    {
                        case ObjectState.Stopped:
                        case ObjectState.Stopping:
                            break;

                        default:
                            throw new Exception("Failed stopping website");
                    }
                }
                if (cfg.AutoStart != null)
                {
                    session.Log($"Configuring site AutoStart to '{cfg.AutoStart}'");
                    site.ServerAutoStart = (bool)cfg.AutoStart;
                    manager.CommitChanges();
                }
                if (cfg.Start)
                {
                    session.Log($"Starting site '{cfg.Website}'");
                    site.Start();
                    manager.CommitChanges();

                    switch (site.State)
                    {
                        case ObjectState.Started:
                        case ObjectState.Starting:
                            break;

                        default:
                            throw new Exception("Failed starting website");
                    }
                }
            }
        }
    }
}