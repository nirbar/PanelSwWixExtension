using Microsoft.Tools.WindowsInstallerXml;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;

namespace PanelSw.Wix.Extensions
{
    class PanelSwWixBinder : BinderExtensionEx
    {
        private List<string> tempFiles_ = new List<string>();

        public override void DatabaseInitialize(Output output)
        {
            base.DatabaseInitialize(output);

            SplitFiles(output);
        }

        ~PanelSwWixBinder()
        {
            // Delete temporary files
            foreach (string f in tempFiles_)
            {
                File.Delete(f);
            }
        }

        private void SplitFiles(Output output)
        {
            // Split root file. Replace source path of root file.
            Table concatFilesTable = output.Tables["PSW_ConcatFiles"];
            if ((concatFilesTable == null) || (concatFilesTable.Rows.Count <= 0))
            {
                return;
            }

            concatFilesTable.Rows.Sort(new ConcatFilesComparer());

            string tmpPath = Path.GetTempPath();
            Table wixFileTable = output.Tables["WixFile"];
            WixFileRow rootWixFile = null;
            int splitSize = Int32.MaxValue;
            FileStream rootFileStream = null;
            try
            {
                foreach (Row currConcatFileRow in concatFilesTable.Rows)
                {
                    // New root file
                    if (!currConcatFileRow[1].Equals(rootWixFile?.File))
                    {
                        splitSize = (int)currConcatFileRow.Fields[4].Data;
                        rootWixFile = Find(wixFileTable, currConcatFileRow.Fields[1].Data) as WixFileRow;
                        if (rootWixFile == null)
                        {
                            Core.OnMessage(WixErrors.WixFileNotFound(currConcatFileRow.Fields[1].Data.ToString()));
                            return;
                        }

                        rootFileStream?.Dispose();
                        rootFileStream = null; // Ensure no double-dispose in case next line throws
                        rootFileStream = File.OpenRead(rootWixFile.Source);

                        string splId = "spl" + Guid.NewGuid().ToString("N");
                        rootWixFile.Source = Path.Combine(tmpPath, splId);
                        tempFiles_.Add(rootWixFile.Source);
                        CopyFilePart(rootFileStream, rootWixFile.Source, splitSize);
                    }

                    WixFileRow currWixFile = Find(wixFileTable, currConcatFileRow.Fields[2].Data) as WixFileRow;
                    if (currWixFile == null)
                    {
                        Core.OnMessage(WixErrors.WixFileNotFound(currConcatFileRow.Fields[2].Data.ToString()));
                        return;
                    }

                    tempFiles_.Add(currWixFile.Source);
                    CopyFilePart(rootFileStream, currWixFile.Source, splitSize);
                }
            }
            finally
            {
                rootFileStream?.Dispose();
            }
        }

        private void CopyFilePart(FileStream srcFile, string dstFile, int copySize)
        {
            byte[] buffer = new byte[1024 * 1024]; // 1MB chunks
            long tmpFileSize = 0;
            using (FileStream dstFileStream = File.OpenWrite(dstFile))
            {
                while ((tmpFileSize < copySize) && (srcFile.Position < srcFile.Length))
                {
                    int chunkSize = (int)Math.Min(copySize - tmpFileSize, buffer.Length);
                    chunkSize = srcFile.Read(buffer, 0, chunkSize);
                    dstFileStream.Write(buffer, 0, chunkSize);
                    tmpFileSize += chunkSize;
                }
            }
        }

        private Row Find(Table table, params object[] keys)
        {
            List<int> keyIndices = new List<int>();
            for (int i = 0; i < table.Definition.Columns.Count; ++i)
            {
                ColumnDefinition col = table.Definition.Columns[i];
                if (col.IsPrimaryKey)
                {
                    keyIndices.Add(i);
                }
            }

            if ((keys == null) || (keys.Length != keyIndices.Count))
            {
                return null;
            }

            foreach (Row row in table.Rows)
            {
                bool match = true;
                for (int i = 0; i < keyIndices.Count; ++i)
                {
                    if (!keys[i].Equals(row.Fields[keyIndices[i]].Data))
                    {
                        match = false;
                        break;
                    }
                }
                if (match)
                {
                    return row;
                }
            }

            return null;
        }


        public override void DatabaseAfterResolvedFields(Output output)
        {
            base.DatabaseAfterResolvedFields(output);

            ResolveTaskScheduler(output);
        }

        private void ResolveTaskScheduler(Output output)
        {
            Table taskScheduler = output.Tables["PSW_TaskScheduler"];
            if (taskScheduler == null)
            {
                return;
            }

            foreach (Row r in taskScheduler.Rows)
            {
                string xmlFile = r[3].ToString();
                if (xmlFile.Contains("!(bindpath."))
                {
                    Core.OnMessage(WixErrors.UnresolvedBindReference(null, "TaskScheduler XmlFile"));
                }
                if (!File.Exists(xmlFile))
                {
                    continue;
                }
                string xml = File.ReadAllText(xmlFile);
                xml = xml.Trim();
                xml = xml.Replace("\r", "");
                xml = xml.Replace("\n", "");
                xml = xml.Replace(Environment.NewLine, "");
                r[3] = xml;
            }
        }
    }

    class ConcatFilesComparer : IComparer
    {
        int IComparer.Compare(Object x, Object y)
        {
            Row r1 = x as Row;
            Row r2 = y as Row;

            int rootFileCmp = r1[1].ToString().CompareTo(r2[1].ToString());
            if (rootFileCmp != 0)
            {
                return rootFileCmp;
            }

            // Same file; Compare by order
            int o1 = (int)r1[3];
            int o2 = (int)r2[3];
            return o1.CompareTo(o2);
        }
    }
}