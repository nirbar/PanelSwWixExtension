using ICSharpCode.SharpZipLib.Core;
using ICSharpCode.SharpZipLib.Zip;
using Microsoft.Deployment.WindowsInstaller;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Xml.Serialization;

namespace PswManagedCA
{
    public class ZipFile
    {
        [Serializable]
        public class ZipFileCatalog
        {
            public ZipFileCatalog()
            {
                Sources = new List<ZipSource>();
            }

            [Serializable]
            public class ZipSource
            {
                public string Id { get; set; }
                public string SrcFolder { get; set; }
                public string Pattern { get; set; }
                public bool Recursive { get; set; }
            }

            public List<ZipSource> Sources { get; set; }

            public string DstZipFile { get; set; }
        }

        private List<ZipFileCatalog> catalogs_ = new List<ZipFileCatalog>();

        [CustomAction]
        public static ActionResult ZipFileSched(Session session)
        {
            AssemblyName me = typeof(JsonJPath).Assembly.GetName();
            session.Log($"Initialized from {me.Name} v{me.Version}");

            ZipFile zipper = new ZipFile();

            using (View view = session.Database.OpenView("SELECT `Id`, `ZipFile`, `CompressFolder`, `FilePattern`, `Recursive`, `Condition` FROM `PSW_ZipFile`"))
            {
                view.Execute(null);

                foreach (Record rec in view)
                {
                    using (rec)
                    {
                        string id = rec.GetString("Id");
                        string zipFile = rec.GetString("ZipFile");
                        string compressFolder = rec.GetString("CompressFolder");
                        string filePattern = rec.GetString("FilePattern");
                        bool recursive = (rec.GetString("Recursive")?.Equals("1") != false); // true or null
                        string condition = rec.GetString("Condition");

                        if (!string.IsNullOrEmpty(condition) && !session.EvaluateCondition(condition))
                        {
                            session.Log($"Condition '{condition}' evaluates to false");
                            continue;
                        }

                        zipper.SchedZip(id, session.Format(zipFile), session.Format(compressFolder), session.Format(filePattern), recursive);
                    }
                }
            }

            if (zipper.catalogs_.Count > 0)
            {
                XmlSerializer srlz = new XmlSerializer(zipper.catalogs_.GetType());
                using (StringWriter sw = new StringWriter())
                {
                    srlz.Serialize(sw, zipper.catalogs_);
                    session["ZipFileExec"] = sw.ToString();
                    session.DoAction("ZipFileExec");
                }
            }

            return ActionResult.Success;
        }

        private void SchedZip(string id, string zipFile, string compressFolder, string filePattern, bool recursive)
        {
            ZipFileCatalog.ZipSource src = new ZipFileCatalog.ZipSource()
            {
                Id = id,
                SrcFolder = compressFolder,
                Pattern = filePattern,
                Recursive = recursive
            };

            zipFile = Path.GetFullPath(zipFile);
            bool isNew = true;
            foreach (ZipFileCatalog ctlg in catalogs_)
            {
                if (Path.GetFullPath(ctlg.DstZipFile).Equals(zipFile, StringComparison.OrdinalIgnoreCase))
                {
                    ctlg.Sources.Add(src);
                    isNew = false;
                    break;
                }
            }

            if (isNew)
            {
                ZipFileCatalog ctlg = new ZipFileCatalog();
                ctlg.DstZipFile = zipFile;
                ctlg.Sources.Add(src);
                catalogs_.Add(ctlg);
            }
        }

        [CustomAction]
        public static ActionResult ZipFileExec(Session session)
        {
            AssemblyName me = typeof(JsonJPath).Assembly.GetName();
            session.Log($"Initialized from {me.Name} v{me.Version}");

            ZipFile zipper = new ZipFile();
            XmlSerializer srlz = new XmlSerializer(zipper.catalogs_.GetType());
            string cad = session["CustomActionData"];
            using (StringReader sr = new StringReader(cad))
            {
                if (srlz.Deserialize(sr) is IEnumerable<ZipFileCatalog> ctlgs)
                {
                    zipper.catalogs_.AddRange(ctlgs);
                }
            }
            zipper.ExecZip(session);

            return ActionResult.Success;
        }

        private void ExecZip(Session session)
        {
            foreach (ZipFileCatalog ctlg in catalogs_)
            {
                session.Log($"Creating ZIP archive '{ctlg.DstZipFile}'");

                string dir = Path.GetDirectoryName(ctlg.DstZipFile);
                if (!Directory.Exists(dir))
                {
                    Directory.CreateDirectory(dir);
                }

                ZipConstants.DefaultCodePage = 850;
                using (FileStream fsOut = File.Create(ctlg.DstZipFile))
                {
                    using (ZipOutputStream zipStream = new ZipOutputStream(fsOut))
                    {
                        zipStream.SetLevel(3); //0-9, 9 being the highest level of compression

                        foreach (ZipFileCatalog.ZipSource src in ctlg.Sources)
                        {
                            if (!Directory.Exists(src.SrcFolder))
                            {
                                continue;
                            }

                            int folderOffset = src.SrcFolder.Length + (src.SrcFolder.EndsWith("\\") ? 0 : 1);

                            List<string> files = new List<string>(Directory.GetFiles(src.SrcFolder, src.Pattern, src.Recursive ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly));
                            if (files.Contains(ctlg.DstZipFile))
                            {
                                files.Remove(ctlg.DstZipFile);
                            }

                            CompressFiles(session, files, zipStream, folderOffset);
                        }
                    }
                }
            }
        }

        private void CompressFiles(Session session, List<string> files, ZipOutputStream zipStream, int folderOffset)
        {
            foreach (string filename in files)
            {
                session.Log($"Adding '{filename}' to ZIP archive");
                FileInfo fi = new FileInfo(filename);

                string entryName = filename.Substring(folderOffset); // Makes the name in zip based on the folder
                entryName = ZipEntry.CleanName(entryName); // Removes drive from name and fixes slash direction
                ZipEntry newEntry = new ZipEntry(entryName);
                newEntry.DateTime = fi.LastWriteTimeUtc;
                newEntry.Size = fi.Length;

                NTTaggedData dates = new NTTaggedData();
                dates.CreateTime = fi.CreationTimeUtc;
                dates.LastAccessTime = fi.LastAccessTimeUtc;
                dates.LastModificationTime = fi.LastWriteTimeUtc;

                ZipExtraData datesData = new ZipExtraData();
                datesData.AddEntry(dates);
                newEntry.ExtraData = datesData.GetEntryData();

                zipStream.PutNextEntry(newEntry);

                byte[] buffer = new byte[4096];
                using (FileStream streamReader = File.OpenRead(filename))
                {
                    StreamUtils.Copy(streamReader, zipStream, buffer);
                }
                zipStream.CloseEntry();
            }
        }
    }
}