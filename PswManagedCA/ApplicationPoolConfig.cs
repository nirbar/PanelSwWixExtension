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
    public class ApplicationPoolConfig
    {
        [Serializable]
        public class ApplicationPoolConfigCatalog
        {
            public string Component { get; set; }
            public string ApplicationPool { get; set; }
            public bool Stop { get; set; } = false;
            public bool Start { get; set; } = false;
            public ErrorHandling ErrorHandling { get; set; } = ErrorHandling.fail;
        }

        [CustomAction]
        public static ActionResult ApplicationPoolConfigSched(Session session)
        {
            AssemblyName me = typeof(ApplicationPoolConfig).Assembly.GetName();
            session.Log($"Initialized from {me.Name} v{me.Version}");

            List<ApplicationPoolConfigCatalog> catalogs = new List<ApplicationPoolConfigCatalog>();
            using (View view = session.Database.OpenView("SELECT `Component_`, `ApplicationPool`, `Stop`, `Start`, `ErrorHandling` FROM `PSW_ApplicationPoolConfig` ORDER BY `Order`"))
            {
                view.Execute(null);

                foreach (Record rec in view)
                {
                    ApplicationPoolConfigCatalog cfg = new ApplicationPoolConfigCatalog();
                    using (rec)
                    {
                        cfg.Component = rec.GetString("Component_");
                        cfg.ApplicationPool = session.Format(rec.GetString("ApplicationPool"));
                        cfg.Stop = (rec.GetInteger("Stop") != 0);
                        cfg.Start = (rec.GetInteger("Start") != 0);
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
                            session.Log($"Component '{ci.Name}' action isn't install, or repair. Skipping ApplicationPoolConfig for '{cfg.ApplicationPool}'");
                            continue;
                    }

                    if (string.IsNullOrEmpty(cfg.ApplicationPool))
                    {
                        session.Log($"ApplicationPool name is empty for component '{ci.Name}'");
                        return ActionResult.Failure;
                    }
                    session.Log($"Will configure application pool '{cfg.ApplicationPool}'");
                    catalogs.Add(cfg);
                }
            }

            if (catalogs.Count > 0)
            {
                XmlSerializer srlz = new XmlSerializer(catalogs.GetType());
                using (StringWriter sw = new StringWriter())
                {
                    srlz.Serialize(sw, catalogs);
                    session["PSW_ApplicationPoolConfigExec"] = sw.ToString();
                    session.DoAction("PSW_ApplicationPoolConfigExec");
                }
            }

            return ActionResult.Success;
        }

        [CustomAction]
        public static ActionResult ApplicationPoolConfigExec(Session session)
        {
            AssemblyName me = typeof(ApplicationPoolConfig).Assembly.GetName();
            session.Log($"Initialized from {me.Name} v{me.Version}");

            List<ApplicationPoolConfigCatalog> actions = new List<ApplicationPoolConfigCatalog>();
            XmlSerializer srlz = new XmlSerializer(actions.GetType());
            string cad = session["CustomActionData"];
            using (StringReader sr = new StringReader(cad))
            {
                IEnumerable<ApplicationPoolConfigCatalog> ctlgs = srlz.Deserialize(sr) as IEnumerable<ApplicationPoolConfigCatalog>;
                if (ctlgs == null)
                {
                    return ActionResult.Success;
                }
                actions.AddRange(ctlgs);
            }

            foreach (ApplicationPoolConfigCatalog ctlg in actions)
            {
            LRetry:
                try
                {
                    ApplicationPoolConfigExec(session, ctlg);
                }
                catch (Exception ex)
                {
                    switch (session.HandleError(ctlg.ErrorHandling, (int)PswErrorMessages.ApplicationPoolConfigFailure, ctlg.ApplicationPool, ex.Message))
                    {
                        default: // Silent / fail
                            session.Log($"User aborted on failure to configure application pool {ctlg.ApplicationPool}. {ex.Message}");
                            return ActionResult.Failure;

                        case MessageResult.Ignore:
                            session.Log($"User ignored failure to configure application pool {ctlg.ApplicationPool}. {ex.Message}");
                            continue;

                        case MessageResult.Retry:
                            session.Log($"User retry on failure to configure application pool {ctlg.ApplicationPool}. {ex.Message}");
                            goto LRetry;
                    }
                }
            }

            return ActionResult.Success;
        }

        private static void ApplicationPoolConfigExec(Session session, ApplicationPoolConfigCatalog cfg)
        {
            using (ServerManager manager = new ServerManager())
            {
                ApplicationPool appPool = manager.ApplicationPools[cfg.ApplicationPool];
                if (appPool == null)
                {
                    throw new Exception($"Could not find '{cfg.ApplicationPool}' application pool");
                }

                if (cfg.Stop)
                {
                    session.Log($"Stopping application pool '{cfg.ApplicationPool}'");
                    appPool.Stop();
                    manager.CommitChanges();

                    switch (appPool.State)
                    {
                        case ObjectState.Stopped:
                        case ObjectState.Stopping:
                            break;

                        default:
                            throw new Exception("Failed stopping application pool");
                    }
                }
                if (cfg.Start)
                {
                    session.Log($"Starting application pool '{cfg.ApplicationPool}'");
                    appPool.Start();
                    manager.CommitChanges();

                    switch (appPool.State)
                    {
                        case ObjectState.Started:
                        case ObjectState.Starting:
                            break;

                        default:
                            throw new Exception("Failed starting application pool");
                    }
                }
            }
        }
    }
}
