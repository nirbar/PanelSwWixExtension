using Microsoft.Deployment.WindowsInstaller;
using Microsoft.Web.Administration;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Xml.Serialization;

namespace PswManagedCA
{
    public class WebsiteConfig
    {
        [Serializable]
        public class WebsiteConfigCatalog
        {
            public string Component { get; set; }
            public string Website { get; set; }
            public bool Stop { get; set; }
        }

        [CustomAction]
        public static ActionResult WebsiteConfigSched(Session session)
        {
            AssemblyName me = typeof(WebsiteConfig).Assembly.GetName();
            session.Log($"Initialized from {me.Name} v{me.Version}");

            List<WebsiteConfigCatalog> catalogs = new List<WebsiteConfigCatalog>();
            using (View view = session.Database.OpenView("SELECT `Component_`, `Website`, `Stop` FROM `PSW_WebsiteConfig`"))
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
                WebsiteConfigExec(session, ctlg);
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
                    site.ServerAutoStart = false;
                    manager.CommitChanges();

                    switch (site.State)
                    {
                        case ObjectState.Stopped:
                        case ObjectState.Stopping:
                            return;

                        default:
                            throw new Exception($"Failed to stop website '{cfg.Website}'- state is '{site.State}'");
                    }
                }
            }
        }
    }
}