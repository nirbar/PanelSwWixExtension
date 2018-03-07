using Microsoft.Deployment.WindowsInstaller;
using PswManagedCA.Util;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Configuration.Install;
using System.IO;
using System.Xml.Serialization;
using System.Linq;

namespace PswManagedCA
{
    public class InstallUtil
    {
        private List<InstallUtilCatalog> catalogs_ = new List<InstallUtilCatalog>();
        private static readonly int msidbComponentAttributes64bit = 0x100;
        enum InstallUtil_Bitness
        {
            asComponent = 0,
            x86 = 1,
            x64 = 2
        }

        [CustomAction]
        public static ActionResult InstallUtilSched(Session session)
        {
            session.Log("Begin InstallUtilSched");

            InstallUtil install_x86 = new InstallUtil();
            InstallUtil remove_x86 = new InstallUtil();
            InstallUtil install_x64 = new InstallUtil();
            InstallUtil remove_x64 = new InstallUtil();

            using (View installUtilView = session.Database.OpenView("SELECT `PSW_InstallUtil`.`File_`, `PSW_InstallUtil`.`Bitness`, `File`.`Component_`, `Component`.`Attributes`"
                + " FROM `PSW_InstallUtil`, `File`, `Component`"
                + " WHERE `PSW_InstallUtil`.`File_`=`File`.`File` AND `File`.`Component_`=`Component`.`Component`"
                ))
            {
                installUtilView.Execute(null);

                foreach (Record installUtilRec in installUtilView)
                {
                    using (installUtilRec)
                    {
                        InstallUtilCatalog ctlg = new InstallUtilCatalog();
                        ctlg.FileId = installUtilRec[1] as string;
                        int explicitBitness = installUtilRec.GetInteger(2);
                        string component = installUtilRec[3] as string;
                        int componentAttr = installUtilRec.GetInteger(4);

                        bool x64 = ((explicitBitness == (int)InstallUtil_Bitness.x64) 
                            || ((explicitBitness == (int)InstallUtil_Bitness.asComponent) && (componentAttr & msidbComponentAttributes64bit) == componentAttr));

                        // Sanity checks
                        if (string.IsNullOrWhiteSpace(ctlg.FileId))
                        {
                            session.Log("File_ not supplied");
                            return ActionResult.Failure;
                        }
                        ComponentInfo ci = session.Components[component];
                        if (ci == null)
                        {
                            session.Log("File '{0}' not present in package", ctlg.FileId);
                            return ActionResult.Failure;
                        }
                        // Path will be empty if component is not scheduled to do anything. We'll check that only if action is relevant.
                        ctlg.FilePath = session.Format(string.Format("[#{0}]", ctlg.FileId));

                        using (View argsView = session.Database.OpenView($"SELECT `Value` FROM `PSW_InstallUtil_Arg` WHERE `File_`='{ctlg.FileId}'"))
                        {
                            argsView.Execute(null);

                            foreach (Record argRec in argsView)
                            {
                                using (argRec)
                                {
                                    string arg = argRec[1] as string;
                                    if (!string.IsNullOrWhiteSpace(arg))
                                    {
                                        arg = session.Format(arg);
                                        if (!string.IsNullOrWhiteSpace(arg))
                                        {
                                            ctlg.Arguments.Add(arg);
                                        }
                                    }
                                }
                            }
                        }

                        switch (ci.RequestState)
                        {
                            case InstallState.Absent:
                            case InstallState.Removed:
                                if (string.IsNullOrWhiteSpace(ctlg.FilePath))
                                {
                                    session.Log("Can't get target path for file '{0}'", ctlg.FileId);
                                    return ActionResult.Failure;
                                }

                                if (x64)
                                {
                                    remove_x64.catalogs_.Add(ctlg);
                                }
                                else
                                {
                                    remove_x86.catalogs_.Add(ctlg);
                                }
                                break;

                            case InstallState.Default:
                            case InstallState.Local:
                            case InstallState.Source:
                                if (string.IsNullOrWhiteSpace(ctlg.FilePath))
                                {
                                    session.Log("Can't get target path for file '{0}'", ctlg.FileId);
                                    return ActionResult.Failure;
                                }
                                if (x64)
                                {
                                    install_x64.catalogs_.Add(ctlg);
                                }
                                else
                                {
                                    install_x86.catalogs_.Add(ctlg);
                                }
                                break;

                            default:
                                session.Log($"Component '{ci.Name}' action isn't install or remove. Skipping InstallUtil for file '{ctlg.FileId}'");
                                continue;
                        }
                    }
                }
            }

            // Install
            install_x64.catalogs_.ForEach((ctlg) => ctlg.Action = InstallUtilCatalog.InstallUtilAction.Install);
            XmlSerializer srlz = new XmlSerializer(install_x64.catalogs_.GetType());
            using (StringWriter sw = new StringWriter())
            {
                srlz.Serialize(sw, install_x64.catalogs_);
                session["PSW_InstallUtil_InstallExec_x64"] = sw.ToString();
            }
            install_x86.catalogs_.ForEach((ctlg) => ctlg.Action = InstallUtilCatalog.InstallUtilAction.Install);
            srlz = new XmlSerializer(install_x86.catalogs_.GetType());
            using (StringWriter sw = new StringWriter())
            {
                srlz.Serialize(sw, install_x86.catalogs_);
                session["PSW_InstallUtil_InstallExec_x86"] = sw.ToString();
            }

            // Install rollback
            install_x64.catalogs_.ForEach((ctlg) => ctlg.Action = InstallUtilCatalog.InstallUtilAction.Uninstall);
            using (StringWriter sw = new StringWriter())
            {
                srlz.Serialize(sw, install_x64.catalogs_);
                session["PSW_InstallUtil_InstallRollback_x64"] = sw.ToString();
            }
            install_x86.catalogs_.ForEach((ctlg) => ctlg.Action = InstallUtilCatalog.InstallUtilAction.Uninstall);
            using (StringWriter sw = new StringWriter())
            {
                srlz.Serialize(sw, install_x86.catalogs_);
                session["PSW_InstallUtil_InstallRollback_x86"] = sw.ToString();
            }

            // Uninstall
            remove_x64.catalogs_.ForEach((ctlg) => ctlg.Action = InstallUtilCatalog.InstallUtilAction.Uninstall);
            using (StringWriter sw = new StringWriter())
            {
                srlz.Serialize(sw, remove_x64.catalogs_);
                session["PSW_InstallUtil_UninstallExec_x64"] = sw.ToString();
            }
            remove_x86.catalogs_.ForEach((ctlg) => ctlg.Action = InstallUtilCatalog.InstallUtilAction.Uninstall);
            using (StringWriter sw = new StringWriter())
            {
                srlz.Serialize(sw, remove_x86.catalogs_);
                session["PSW_InstallUtil_UninstallExec_x86"] = sw.ToString();
            }

            // Uninstall rollback
            remove_x64.catalogs_.ForEach((ctlg) => ctlg.Action = InstallUtilCatalog.InstallUtilAction.Install);
            using (StringWriter sw = new StringWriter())
            {
                srlz.Serialize(sw, remove_x64.catalogs_);
                session["PSW_InstallUtil_UninstallRollback_x64"] = sw.ToString();
            }
            remove_x86.catalogs_.ForEach((ctlg) => ctlg.Action = InstallUtilCatalog.InstallUtilAction.Install);
            using (StringWriter sw = new StringWriter())
            {
                srlz.Serialize(sw, remove_x86.catalogs_);
                session["PSW_InstallUtil_UninstallRollback_x86"] = sw.ToString();
            }

            return ActionResult.Success;
        }

        /// <summary>
        /// Deferred execution of InstallUtil.
        /// This is the entry point for all InstallUtil actions (install, remove and their respective rollback).
        /// </summary>
        /// <param name="session"></param>
        /// <returns></returns>
        [CustomAction]
        public static ActionResult InstallUtilExec(Session session)
        {
            session.Log("Begin InstallUtilExec");

            InstallUtil executer = new InstallUtil();

            XmlSerializer srlz = new XmlSerializer(executer.catalogs_.GetType());
            string cad = session["CustomActionData"];
            using (StringReader sr = new StringReader(cad))
            {
                if (srlz.Deserialize(sr) is IEnumerable<InstallUtilCatalog> ctlgs)
                {
                    executer.catalogs_.AddRange(ctlgs);
                }
            }
            executer.Execute(session);

            return ActionResult.Success;
        }

        private void Execute(Session session)
        {
            foreach (InstallUtilCatalog ctlg in catalogs_)
            {
                // Temporary file for logging
                string tmpFile = Path.GetTempFileName();
                IDictionary savedState = new Hashtable();

                try
                {
                    // (Un)Install the assembly
                    if (ctlg.Arguments.Count > 0)
                    {
                        session.Log($"Applying {ctlg.Action} on assembly '{ctlg.FilePath}' with arguments {ctlg.Arguments.Aggregate((a, c) => $"{a} {c}")}");
                    }
                    else
                    {
                        session.Log($"Applying {ctlg.Action} on assembly '{ctlg.FilePath}'");
                    }

                    ctlg.Arguments.Add($"/LogFile={tmpFile}");
                    ctlg.Arguments.Add("/LogToConsole=false");

                    AssemblyInstaller installer = new AssemblyInstaller(ctlg.FilePath, ctlg.Arguments.ToArray());
                    installer.UseNewContext = true;

                    switch (ctlg.Action)
                    {
                        case InstallUtilCatalog.InstallUtilAction.Install:
                            installer.Install(savedState);
                            installer.Commit(savedState);
                            break;

                        case InstallUtilCatalog.InstallUtilAction.Uninstall:
                            installer.Uninstall(savedState);
                            break;
                    }
                }
                finally
                {
                    // Dump and clear log
                    if (File.Exists(tmpFile))
                    {
                        session.Log(File.ReadAllText(tmpFile));
                        File.Delete(tmpFile);
                    }
                }
            }
        }

        [Serializable]
        public class InstallUtilCatalog
        {
            public InstallUtilCatalog()
            {
                Arguments = new List<string>();
            }

            public enum InstallUtilAction
            {
                Install,
                Uninstall
            }

            public InstallUtilAction Action { get; set; }

            public List<string> Arguments { get; set; }

            public string FileId { get; set; }

            public string FilePath { get; set; }
        }
    }
}
