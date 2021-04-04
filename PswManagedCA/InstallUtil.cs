using Microsoft.Deployment.WindowsInstaller;
using PswManagedCA.Util;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Configuration.Install;
using System.IO;
using System.Xml.Serialization;
using System.Linq;
using System.ComponentModel;
using System.Reflection;

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

        enum ServiceErrorCode
        {
            ERROR_SERVICE_DOES_NOT_EXIST = 1060,
            ERROR_SERVICE_MARKED_FOR_DELETE = 1072,
            ERROR_SERVICE_EXISTS = 1073,
        }

        [CustomAction]
        public static ActionResult InstallUtilSched(Session session)
        {
            AssemblyName me = typeof(JsonJPath).Assembly.GetName();
            session.Log($"Initialized from {me.Name} v{me.Version}");

            InstallUtil all = new InstallUtil();

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
                        ctlg.ComponentInfo = ci;
                        ctlg.X64 = x64;

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
                                        string obfuscated = session.Obfuscate(arg);
                                        arg = session.Format(arg);
                                        if (!string.IsNullOrWhiteSpace(arg))
                                        {
                                            ctlg.Arguments.Add(arg);
                                            ctlg.ObfuscatedArguments.Add(obfuscated);
                                        }
                                    }
                                }
                            }
                        }

                        switch (ci.RequestState)
                        {
                            case InstallState.Absent:
                            case InstallState.Removed:
                            case InstallState.Default:
                            case InstallState.Local:
                            case InstallState.Source:
                                if (string.IsNullOrWhiteSpace(ctlg.FilePath))
                                {
                                    session.Log("Can't get target path for file '{0}'", ctlg.FileId);
                                    return ActionResult.Failure;
                                }

                                all.catalogs_.Add(ctlg);
                                break;

                            default:
                                session.Log($"Component '{ci.Name}' action isn't install, repair or remove. Skipping InstallUtil for file '{ctlg.FileId}'");
                                continue;
                        }
                    }
                }
            }

            // Install has rollback to uninstall
            // Uninstall has rollback to install
            // Repair has no rollbak
            XmlSerializer srlz = new XmlSerializer(all.catalogs_.GetType());
            List<InstallUtilCatalog> temp = new List<InstallUtilCatalog>();

            // Install + repair x86
            temp.AddRange(from c in all.catalogs_
                          where (!c.X64 && (c.ComponentInfo.RequestState >= InstallState.Local))
                          select c);
            if (temp.Count() > 0)
            {                
                foreach(InstallUtilCatalog c in temp)
                {
                    c.Action = (c.ComponentInfo.CurrentState < InstallState.Local) ? InstallUtilCatalog.InstallUtilAction.Install : InstallUtilCatalog.InstallUtilAction.Reinstall;
                }
                using (StringWriter sw = new StringWriter())
                {
                    srlz.Serialize(sw, temp);
                    session["PSW_InstallUtil_InstallExec_x86"] = sw.ToString();
                }
            }
            // Rollback of install x86
            temp.Clear();
            temp.AddRange(from c in all.catalogs_
                          where (!c.X64 && ((c.ComponentInfo.RequestState >= InstallState.Local) && (c.ComponentInfo.CurrentState < InstallState.Local)))
                          select c);
            if (temp.Count() > 0)
            {
                foreach (InstallUtilCatalog c in temp)
                {
                    c.Action = InstallUtilCatalog.InstallUtilAction.Uninstall;
                }
                using (StringWriter sw = new StringWriter())
                {
                    srlz.Serialize(sw, temp);
                    session["PSW_InstallUtil_InstallRollback_x86"] = sw.ToString();
                }
            }
            // Install + repair x64
            temp.Clear();
            temp.AddRange(from c in all.catalogs_
                          where (c.X64 && (c.ComponentInfo.RequestState >= InstallState.Local))
                          select c);
            if (temp.Count() > 0)
            {
                foreach (InstallUtilCatalog c in temp)
                {
                    c.Action = (c.ComponentInfo.CurrentState < InstallState.Local) ? InstallUtilCatalog.InstallUtilAction.Install : InstallUtilCatalog.InstallUtilAction.Reinstall;
                }
                using (StringWriter sw = new StringWriter())
                {
                    srlz.Serialize(sw, temp);
                    session["PSW_InstallUtil_InstallExec_x64"] = sw.ToString();
                }
            }
            // Rollback of install x64
            temp.Clear();
            temp.AddRange(from c in all.catalogs_
                          where (c.X64 && ((c.ComponentInfo.RequestState >= InstallState.Local) && (c.ComponentInfo.CurrentState < InstallState.Local)))
                          select c);
            if (temp.Count() > 0)
            {
                foreach (InstallUtilCatalog c in temp)
                {
                    c.Action = InstallUtilCatalog.InstallUtilAction.Uninstall;
                }
                using (StringWriter sw = new StringWriter())
                {
                    srlz.Serialize(sw, temp);
                    session["PSW_InstallUtil_InstallRollback_x64"] = sw.ToString();
                }
            }

            // UnInstall x86
            temp.Clear();
            temp.AddRange(from c in all.catalogs_
                          where (!c.X64 && (c.ComponentInfo.CurrentState >= InstallState.Local) && (c.ComponentInfo.RequestState < InstallState.Local))
                          select c);
            if (temp.Count() > 0)
            {
                foreach (InstallUtilCatalog c in temp)
                {
                    c.Action = InstallUtilCatalog.InstallUtilAction.Uninstall;
                }
                using (StringWriter sw = new StringWriter())
                {
                    srlz.Serialize(sw, temp);
                    session["PSW_InstallUtil_UninstallExec_x86"] = sw.ToString();
                }
                foreach (InstallUtilCatalog c in temp)
                {
                    c.Action = InstallUtilCatalog.InstallUtilAction.Install;
                }
                using (StringWriter sw = new StringWriter())
                {
                    srlz.Serialize(sw, temp);
                    session["PSW_InstallUtil_UninstallRollback_x86"] = sw.ToString();
                }
            }
            // UnInstall x64
            temp.Clear();
            temp.AddRange(from c in all.catalogs_
                          where (c.X64 && (c.ComponentInfo.CurrentState >= InstallState.Local) && (c.ComponentInfo.RequestState < InstallState.Local))
                          select c);
            if (temp.Count() > 0)
            {
                foreach (InstallUtilCatalog c in temp)
                {
                    c.Action = InstallUtilCatalog.InstallUtilAction.Uninstall;
                }
                using (StringWriter sw = new StringWriter())
                {
                    srlz.Serialize(sw, temp);
                    session["PSW_InstallUtil_UninstallExec_x64"] = sw.ToString();
                }
                foreach (InstallUtilCatalog c in temp)
                {
                    c.Action = InstallUtilCatalog.InstallUtilAction.Install;
                }
                using (StringWriter sw = new StringWriter())
                {
                    srlz.Serialize(sw, temp);
                    session["PSW_InstallUtil_UninstallRollback_x64"] = sw.ToString();
                }
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
            AssemblyName me = typeof(JsonJPath).Assembly.GetName();
            session.Log($"Initialized from {me.Name} v{me.Version}");

            InstallUtil executer = new InstallUtil();

            XmlSerializer srlz = new XmlSerializer(executer.catalogs_.GetType());
            string cad = session["CustomActionData"];
            if (string.IsNullOrWhiteSpace(cad))
            {
                session.Log("Nothing to do");
                return ActionResult.Success;
            }

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
                    if (ctlg.ObfuscatedArguments.Count > 0)
                    {
                        session.Log($"Applying {ctlg.Action} on assembly '{ctlg.FilePath}' with arguments {ctlg.ObfuscatedArguments.Aggregate((a, c) => $"{a} {c}")}");
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
                        case InstallUtilCatalog.InstallUtilAction.Reinstall:
                            installer.Install(savedState);
                            installer.Commit(savedState);
                            break;

                        case InstallUtilCatalog.InstallUtilAction.Uninstall:
                            installer.Uninstall(savedState);
                            break;
                    }
                }
                catch (Win32Exception ex)
                {
                    // Ignore if:
                    // - Deleting and service doesn't exist
                    // - Deleting and service is marked for delete
                    // - Repairing and service is already installed
                    switch (ex.NativeErrorCode)
                    {
                        case (int)ServiceErrorCode.ERROR_SERVICE_DOES_NOT_EXIST:
                            if (ctlg.Action != InstallUtilCatalog.InstallUtilAction.Uninstall)
                            {
                                throw;
                            }
                            session.Log($"Service {ctlg.FilePath} is not installed anyway");
                            break;

                        case (int)ServiceErrorCode.ERROR_SERVICE_EXISTS:
                            if (ctlg.Action != InstallUtilCatalog.InstallUtilAction.Reinstall)
                            {
                                throw;
                            }
                            session.Log($"Service {ctlg.FilePath} is already installed");
                            break;

                        case (int)ServiceErrorCode.ERROR_SERVICE_MARKED_FOR_DELETE:
                            if (ctlg.Action != InstallUtilCatalog.InstallUtilAction.Uninstall)
                            {
                                throw;
                            }
                            session.Log($"Service {ctlg.FilePath} is already marked for delete");
                            break;

                        default:
                            session.Log($"Exception with code {ex.ErrorCode} (native {ex.NativeErrorCode}): {ex}");
                            throw;
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
                ObfuscatedArguments = new List<string>();
            }

            public enum InstallUtilAction
            {
                Install,
                Reinstall,
                Uninstall
            }

            public InstallUtilAction Action { get; set; }

            public List<string> Arguments { get; set; }

            public List<string> ObfuscatedArguments { get; set; }

            public string FileId { get; set; }

            public string FilePath { get; set; }

            [XmlIgnore]
            public ComponentInfo ComponentInfo { get; set; }

            [XmlIgnore]
            public bool X64 { get; set; }
        }
    }
}
