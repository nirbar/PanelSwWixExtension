using ICSharpCode.SharpZipLib.Core;
using ICSharpCode.SharpZipLib.Zip;
using Microsoft.Deployment.WindowsInstaller;
using System;
using System.Collections.Generic;
using System.IO;
using System.Xml.Serialization;

namespace PswManagedCA
{
    public class Unzip
    {
        [Serializable]
        public class UnzipCatalog
        {
            public string Id { get; set; }
            public string ZipFile { get; set; }
            public string TargetFolder { get; set; }
            public UnzipFlags Flags { get; set; }
        }

        [Flags]
        public enum UnzipFlags
        {
            None = 0,
            Overwrite = 1
        }

        private List<UnzipCatalog> catalogs_ = new List<UnzipCatalog>();

        [CustomAction]
        public static ActionResult UnzipSched(Session session)
        {
            session.Log("Begin UnzipSched");
            Unzip unzipper = new Unzip();

            IList<string> results = session.Database.ExecuteStringQuery("SELECT `Id`, `ZipFile`, `TargetFolder`, `Flags`, `Condition` FROM `PSW_Unzip`");
            for (int i = 0; i < results.Count; i += 5)
            {
                string id = results[i + 0]?.ToString();
                string zipFile = results[i + 1]?.ToString();
                string targetFolder = results[i + 2]?.ToString();
                string flags = results[i + 3]?.ToString();
                string condition = results[i + 4]?.ToString();

                if (!string.IsNullOrEmpty(condition) && !session.EvaluateCondition(condition))
                {
                    session.Log($"Condition '{condition}' evaluates to false");
                    continue;
                }

                int nFlags = int.Parse(flags);
                unzipper.SchedUnzip(id, session.Format(zipFile), session.Format(targetFolder), (UnzipFlags)nFlags);
            }

            XmlSerializer srlz = new XmlSerializer(unzipper.catalogs_.GetType());
            using (StringWriter sw = new StringWriter())
            {
                srlz.Serialize(sw, unzipper.catalogs_);
                session["UnzipExec"] = sw.ToString();
            }

            return ActionResult.Success;
        }

        private void SchedUnzip(string id, string zipFile, string targetFolder, UnzipFlags flags)
        {
            zipFile = Path.GetFullPath(zipFile);
            UnzipCatalog ctlg = new UnzipCatalog()
            {
                Id = id,
                TargetFolder = targetFolder,
                ZipFile = zipFile,
                Flags = flags,
            };

            catalogs_.Add(ctlg);
        }

        [CustomAction]
        public static ActionResult UnzipExec(Session session)
        {
            session.Log("Begin UnzipExec");

            Unzip unzipper = new Unzip();
            XmlSerializer srlz = new XmlSerializer(unzipper.catalogs_.GetType());
            string cad = session["CustomActionData"];
            using (StringReader sr = new StringReader(cad))
            {
                if (srlz.Deserialize(sr) is IEnumerable<UnzipCatalog> ctlgs)
                {
                    unzipper.catalogs_.AddRange(ctlgs as IEnumerable<UnzipCatalog>);
                }
            }
            unzipper.ExecUnzip(session);

            return ActionResult.Success;
        }

        private void ExecUnzip(Session session)
        {
            foreach (UnzipCatalog ctlg in catalogs_)
            {
                session.Log($"Extracting ZIP archive '{ctlg.ZipFile}' to folder '{ctlg.TargetFolder}");

                if (!File.Exists(ctlg.ZipFile))
                {
                    session.Log($"ZIP archive does not exist: {ctlg.ZipFile}");
                    continue;
                }

                if (!Directory.Exists(ctlg.TargetFolder))
                {
                    Directory.CreateDirectory(ctlg.TargetFolder);
                }

                using (FileStream fs = File.OpenRead(ctlg.ZipFile))
                {
                    using (ICSharpCode.SharpZipLib.Zip.ZipFile zf = new ICSharpCode.SharpZipLib.Zip.ZipFile(fs))
                    {
                        foreach (ZipEntry zipEntry in zf)
                        {
                            if (!zipEntry.IsFile)
                            {
                                continue;           // Ignore directories
                            }
                            String entryFileName = zipEntry.Name;
                            byte[] buffer = new byte[4096];     // 4K is optimum
                            Stream zipStream = zf.GetInputStream(zipEntry);

                            String fullZipToPath = Path.Combine(ctlg.TargetFolder, entryFileName);
                            string directoryName = Path.GetDirectoryName(fullZipToPath);
                            if (directoryName.Length > 0)
                            {
                                Directory.CreateDirectory(directoryName);
                            }

                            if (File.Exists(fullZipToPath))
                            {
                                if ((ctlg.Flags & UnzipFlags.Overwrite) != UnzipFlags.Overwrite)
                                {
                                    session.Log($"Skipping existing file '{fullZipToPath}'");
                                    continue;
                                }

                                session.Log($"Overwriting existing file '{fullZipToPath}'");
                                File.SetAttributes(fullZipToPath, System.IO.FileAttributes.Normal);
                                File.Delete(fullZipToPath);
                            }

                            using (FileStream streamWriter = File.Create(fullZipToPath))
                            {
                                StreamUtils.Copy(zipStream, streamWriter, buffer);
                            }
                        }
                    }
                }
            }
        }
    }
}