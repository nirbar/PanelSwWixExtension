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
        private enum JsonFormatting
        {
            Raw,
            String,
            Boolean
        }

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

            using (View jsonView = session.Database.OpenView("SELECT `PSW_JsonJPath`.`File_`, `PSW_JsonJPath`.`Component_`, `PSW_JsonJPath`.`FilePath`, `PSW_JsonJPath`.`JPath`, `PSW_JsonJPath`.`Value`, `PSW_JsonJPath`.`Formatting`, `PSW_JsonJPath`.`ErrorHandling` FROM `PSW_JsonJPath`"))
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
                        int? formatting = rec[6] as int?;
                        int? errorHandling = rec[7] as int?;
                        ctlg.JPathObfuscated = session.Obfuscate(jpath);
                        ctlg.ValueObfuscated = session.Obfuscate(value);
                        ctlg.JPath = session.Format(jpath);
                        ctlg.Value = session.Format(value);
                        bool isHidden = !ctlg.Value.Equals(ctlg.ValueObfuscated);

                        if ((formatting != null) && Enum.IsDefined(typeof(JsonFormatting), (int)formatting))
                        {
                            JsonFormatting jsonFormat = (JsonFormatting)formatting;
                            switch (jsonFormat)
                            {
                                case JsonFormatting.Raw:
                                default:
                                    break;

                                case JsonFormatting.String:
                                    ctlg.Value = JsonConvert.ToString(ctlg.Value);
                                    break;

                                case JsonFormatting.Boolean:
                                    if (string.IsNullOrWhiteSpace(ctlg.Value))
                                    {
                                        ctlg.Value = JsonConvert.False;
                                    }
                                    else if (bool.TryParse(ctlg.Value, out bool b))
                                    {
                                        ctlg.Value = b ? JsonConvert.True : JsonConvert.False;
                                    }
                                    else if (int.TryParse(ctlg.Value, out int i) && (i == 0))
                                    {
                                        ctlg.Value = JsonConvert.False;
                                    }
                                    else
                                    {
                                        ctlg.Value = JsonConvert.True;
                                    }
                                    break;
                            }
                            if (!isHidden)
                            {
                                ctlg.ValueObfuscated = ctlg.Value;
                            }
                        }

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
                        if (errorHandling != null)
                        {
                            ctlg.ErrorHandling = (Util.ErrorHandling)errorHandling;
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
            return executer.Execute(session);
        }

        private ActionResult Execute(Session session)
        {
            foreach (JsonJPathCatalog ctlg in catalogs_)
            {
                LRetry:
                try
                {
                    JObject jo = JObject.Parse(File.ReadAllText(ctlg.FilePath));
                    JToken token = jo.SelectToken(ctlg.JPath, true);
                    if (token == null)
                    {
                        throw new Exception("JPath did not match any results");
                    }

                    token.Replace(JToken.Parse(ctlg.Value));
                    File.WriteAllText(ctlg.FilePath, jo.ToString(Formatting.Indented));
                }
                catch (Exception ex)
                {
                    session.LogUnformatted($"Failed setting JsonJpath '{ctlg.JPathObfuscated}' to '{ctlg.ValueObfuscated}' in file '{ctlg.FilePath}': {ex}");
                    switch( session.HandleError(ctlg.ErrorHandling, 27009, ctlg.JPathObfuscated, ctlg.ValueObfuscated, ctlg.FilePath, ex.Message))
                    {
                        case MessageResult.Abort:
                            session.Log($"Aborted on failure");
                            return ActionResult.Failure;

                        case MessageResult.Ignore:
                            session.Log($"Ignored failure");
                            continue;

                        case MessageResult.Retry:
                            session.Log($"User retried on failure");
                            goto LRetry;
                    }
                }
            }
            return ActionResult.Success;
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

        public ErrorHandling ErrorHandling { get; set; } = ErrorHandling.fail;
    }
}