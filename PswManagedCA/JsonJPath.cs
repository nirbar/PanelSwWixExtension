using Microsoft.Deployment.WindowsInstaller;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Xml.Serialization;
using PswManagedCA.Util;

namespace PswManagedCA
{
    public class JsonJPath
    {
        List<JsonJPathCatalog> catalogs_ = new List<JsonJPathCatalog>();

        [CustomAction]
        public static ActionResult JsonJpathSearch(Session session)
        {
            AssemblyName me = typeof(JsonJPath).Assembly.GetName();
            session.Log($"Initialized from {me.Name} v{me.Version}");

            using (View jsonView = session.Database.OpenView("SELECT `Property_`, `FilePath`, `JPath` FROM `PSW_JsonJpathSearch`"))
            {
                jsonView.Execute(null);

                foreach (Record rec in jsonView)
                {
                    using (rec)
                    {
                        string property = rec[1] as string;
                        string file = session.Format(rec[2] as string);
                        string jpathUnformatted = rec[3] as string;
                        string jpath = session.Format(jpathUnformatted);

                        // Sanity checks
                        if (string.IsNullOrWhiteSpace(property))
                        {
                            session.Log("Property_ not supplied");
                            return ActionResult.Failure;
                        }
                        session.LogObfuscated($"Running Expression '{jpathUnformatted}' on '{file}'");
                        if (string.IsNullOrWhiteSpace(file) || !File.Exists(file))
                        {
                            session.Log($"File not found: '{file}'");
                            continue;
                        }

                        JObject jo = JObject.Parse(File.ReadAllText(file));
                        JToken token = jo.SelectToken(jpath, false);
                        if ((token != null) && (token.Type != JTokenType.Null))
                        {
                            session[property] = token.ToString(Formatting.None);
                        }
                    }
                }
            }
            return ActionResult.Success;
        }

        [CustomAction]
        public static ActionResult JsonJpathSched(Session session)
        {
            AssemblyName me = typeof(JsonJPath).Assembly.GetName();
            session.Log($"Initialized from {me.Name} v{me.Version}");

            List<JsonJPathCatalog> catalogs = new List<JsonJPathCatalog>(); ;

            using (View jsonView = session.Database.OpenView("SELECT `PSW_JsonJPath`.`File_`, `PSW_JsonJPath`.`Component_`, `PSW_JsonJPath`.`FilePath`, `PSW_JsonJPath`.`JPath`, `PSW_JsonJPath`.`Value` FROM `PSW_JsonJPath`"))
            {
                jsonView.Execute(null);

                foreach (Record rec in jsonView)
                {
                    using (rec)
                    {
                        JsonJPathCatalog ctlg = new JsonJPathCatalog();
                        string fileId = rec[1] as string;
                        string component = rec[2] as string;
                        string filePath = rec[3] as string;
                        string jpath = rec[4] as string;
                        string value = rec[5] as string;
                        ctlg.JPathObfuscated = session.Obfuscate(jpath);
                        ctlg.ValueObfuscated = session.Obfuscate(value);
                        ctlg.JPath = session.Format(jpath);
                        ctlg.Value = session.Format(value);

                        // Sanity checks
                        if (string.IsNullOrWhiteSpace(fileId) == string.IsNullOrEmpty(component))
                        {
                            session.Log("Either File_ or Component and FilePath must be supplied");
                            return ActionResult.Failure;
                        }
                        if (string.IsNullOrWhiteSpace(filePath) != string.IsNullOrEmpty(component))
                        {
                            session.Log("Either File_ or Component and FilePath must be supplied");
                            return ActionResult.Failure;
                        }

                        if (string.IsNullOrEmpty(fileId))
                        {
                            ctlg.FilePath = session.Format(filePath);
                        }
                        else // Get component by file Id
                        {
                            ctlg.FilePath = session.Format($"[#{fileId}]");
                            using (View componentView = session.Database.OpenView($"SELECT `Component_` FROM `File` WHERE `File`='{fileId}'"))
                            {
                                componentView.Execute(null);
                                foreach (Record rec1 in componentView)
                                {
                                    using (rec1)
                                    {
                                        component = rec1[1] as string;
                                    }
                                }
                            }
                        }
                        if (string.IsNullOrWhiteSpace(component))
                        {
                            session.Log("Did not find component");
                            return ActionResult.Failure;
                        }

                        ComponentInfo ci = session.Components[component];
                        if (ci == null)
                        {
                            session.Log($"Component '{component}' not present in package");
                            return ActionResult.Failure;
                        }
                        // Path will be empty if component is not scheduled to do anything. We'll check that only if action is relevant.

                        switch (ci.RequestState)
                        {
                            case InstallState.Default:
                            case InstallState.Local:
                            case InstallState.Source:
                                if (string.IsNullOrWhiteSpace(ctlg.FilePath))
                                {
                                    session.Log("Can't get target path for file");
                                    return ActionResult.Failure;
                                }

                                session.LogUnformatted($"Will replace JSON token matching JPath '{ctlg.JPathObfuscated}' with '{ctlg.ValueObfuscated}' in file '{ctlg.FilePath}'");
                                catalogs.Add(ctlg);
                                break;

                            default:
                                session.Log($"Component '{ci.Name}' action isn't install, or repair. Skipping JPath");
                                continue;
                        }
                    }
                }
            }

            if (catalogs.Count > 0)
            {
                XmlSerializer srlz = new XmlSerializer(catalogs.GetType());
                using (StringWriter sw = new StringWriter())
                {
                    srlz.Serialize(sw, catalogs);
                    session["JsonJpathExec"] = sw.ToString();
                    session.DoAction("JsonJpathExec");
                }
            }

            return ActionResult.Success;
        }

        [CustomAction]
        public static ActionResult JsonJpathExec(Session session)
        {
            AssemblyName me = typeof(JsonJPath).Assembly.GetName();
            session.Log($"Initialized from {me.Name} v{me.Version}");

            JsonJPath executer = new JsonJPath();
            XmlSerializer srlz = new XmlSerializer(executer.catalogs_.GetType());
            string cad = session["CustomActionData"];
            if (string.IsNullOrWhiteSpace(cad))
            {
                session.Log("Nothing to do");
                return ActionResult.Success;
            }

            using (StringReader sr = new StringReader(cad))
            {
                if (srlz.Deserialize(sr) is IEnumerable<JsonJPathCatalog> ctlgs)
                {
                    executer.catalogs_.AddRange(ctlgs);
                }
            }
            executer.Execute(session);

            return ActionResult.Success;
        }

        private void Execute(Session session)
        {
            foreach (JsonJPathCatalog ctlg in catalogs_)
            {
                try
                {
                    JObject jo = JObject.Parse(File.ReadAllText(ctlg.FilePath));
                    JToken token = jo.SelectToken(ctlg.JPath, true);
                    if (token == null)
                    {
                        session.LogUnformatted($"Did not find results for Jpath '{ctlg.JPathObfuscated}' in file '{ctlg.FilePath}'");
                        continue;
                    }

                    token.Replace(JToken.Parse(ctlg.Value));
                    File.WriteAllText(ctlg.FilePath, jo.ToString(Formatting.Indented));
                }
                catch (Exception ex)
                {
                    session.LogUnformatted($"Failed setting JsonJpath '{ctlg.JPathObfuscated}' to '{ctlg.ValueObfuscated}' in file '{ctlg.FilePath}': {ex.Message}");
                    throw;
                }
            }
        }
    }

    [Serializable]
    public class JsonJPathCatalog
    {
        public string FilePath { get; set; }

        public string JPath { get; set; }

        public string JPathObfuscated { get; set; }

        public string ValueObfuscated { get; set; }

        public string Value { get; set; }
    }
}