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

        private IEnumerable<Row> Select(Table table, Func<Row, bool> predicate)
        {
            List<Row> results = new List<Row>();
            foreach (Row r in table.Rows)
            {
                if (predicate(r))
                {
                    results.Add(r);
                }
            }
            return results;
        }

        private Row FindOne(Table table, Func<Row, bool> predicate)
        {
            foreach (Row r in table.Rows)
            {
                if (predicate(r))
                {
                    return r;
                }
            }
            return null;
        }

        public override void DatabaseAfterResolvedFields(Output output)
        {
            base.DatabaseAfterResolvedFields(output);

            ResolveTaskScheduler(output);
            DuplicateFolder(output);
            CheckExecuteCommandSequences(output);
            ValidateSingleRemoveFolderExLongPathHandling(output);
        }

        private void CheckExecuteCommandSequences(Output output)
        {
            Table executeCommands = output.Tables["PSW_ExecuteCommand"];
            if ((executeCommands == null) || (executeCommands.Rows.Count <= 0))
            {
                return;
            }

            Table wixActions = output.Tables["WixAction"];
            foreach (Row executeCommand in executeCommands.Rows)
            {
                string id = executeCommand[0].ToString();
                string prepareId = $"Prepare{id}";
                string schedId = $"Sched{id}";

                // Actions rows:
                // field 0: Sequence
                // field 1: Id
                // field 4: Before
                // field 5: After

                IEnumerable<Row> actionRows = Select(wixActions, r => "InstallExecuteSequence".Equals(r[0]) && (
                (id.Equals(r[4]) && !schedId.Equals(r[1])) // Actions that might be sequnced before PSW_ExecuteCommand
                || (schedId.Equals(r[4]) && !prepareId.Equals(r[1])) // Before Sched, but not Prepare
                || (schedId.Equals(r[5]) && !id.Equals(r[1])) // After Sched, but not PSW_ExecuteCommand
                || (prepareId.Equals(r[5]) && !schedId.Equals(r[1])) // After Prepare, but not Sched
                ));
                foreach (Row action in actionRows)
                {
                    Core.OnMessage(PanelSwWixErrorMessages.ExecuteCommandSequence(action.SourceLineNumbers, id, action[1].ToString()));
                }
            }
        }

        private void ValidateSingleRemoveFolderExLongPathHandling(Output output)
        {
            Table removeFolderEx = output.Tables["PSW_RemoveFolderEx"];
            if ((removeFolderEx == null) || (removeFolderEx.Rows.Count <= 0))
            {
                return;
            }

            // Collect temporary file paths to later delete
            Row nonDefault = null;
            foreach (Row rmf in removeFolderEx.Rows)
            {
                if ((int)rmf[4] == (int)PanelSwWixCompiler.RemoveFolderExLongPathHandling.Default)
                {
                    continue;
                }
                if (nonDefault == null)
                {
                    nonDefault = rmf;
                    continue;
                }
                if (nonDefault[4] != rmf[4])
                {
                    Core.OnMessage(PanelSwWixErrorMessages.MismatchingRemoveFolderExLongPathHandling(rmf.SourceLineNumbers));
                    Core.OnMessage(PanelSwWixErrorMessages.MismatchingRemoveFolderExLongPathHandling(nonDefault.SourceLineNumbers));
                }
            }
        }

        private void DuplicateFolder(Output output)
        {
            Table duplicateFolders = output.Tables["PSW_DuplicateFolder"];
            if ((duplicateFolders == null) || (duplicateFolders.Rows.Count <= 0))
            {
                return;
            }

            Table duplicateFiles = output.Tables["DuplicateFile"];
            Table createFolders = output.Tables["CreateFolder"];
            Table components = output.Tables["Component"];
            Table files = output.Tables["File"];
            Table directories = output.Tables["Directory"];

            // Collect temporary file paths to later delete
            foreach (Row dup in duplicateFolders.Rows)
            {
                DuplicateFolder(duplicateFiles, components, files, directories, createFolders, dup.SourceLineNumbers, dup[0].ToString(), dup[1].ToString());
            }
        }

        private void DuplicateFolder(Table duplicateFiles, Table components, Table files, Table directories, Table createFolders, SourceLineNumberCollection sourceLineNumber, string sourceDir, string dstDir)
        {
            // Duplicate files in source dir
            IEnumerable<Row> dirComponents = Select(components, c => c[2].Equals(sourceDir));
            foreach (Row component in dirComponents)
            {
                IEnumerable<Row> compFiles = Select(files, f => f[1].Equals(component[0]));
                foreach (Row file in compFiles)
                {
                    Row createFolder = Find(createFolders, dstDir, component[0]);
                    if (createFolder == null)
                    {
                        createFolder = new Row(sourceLineNumber, createFolders);
                        createFolder[0] = dstDir;
                        createFolder[1] = component[0];
                        createFolders.Rows.Add(createFolder);
                    }

                    Row duplicateFile = FindOne(duplicateFiles, d => d[1].Equals(component[0]) && file[0].Equals(d[2]) && dstDir.Equals(d[4]));
                    if (duplicateFile == null)
                    {
                        duplicateFile = new Row(sourceLineNumber, duplicateFiles);
                        duplicateFile[0] = Core.GenerateIdentifier("dpf", component[0].ToString(), file[0].ToString(), dstDir);
                        duplicateFile[1] = component[0];
                        duplicateFile[2] = file[0];
                        duplicateFile[4] = dstDir;

                        duplicateFiles.Rows.Add(duplicateFile);
                    }
                }
            }

            // Duplicate sub folders
            IEnumerable<Row> subdirs = Select(directories, d => sourceDir.Equals(d[1]));
            foreach (Row dir in subdirs)
            {
                Row destChildDir = FindOne(directories, d => d[2].Equals(dir[2]) && dstDir.Equals(d[1]));
                if (destChildDir == null)
                {
                    destChildDir = new Row(sourceLineNumber, directories);
                    destChildDir[0] = Core.GenerateIdentifier("dpd", dstDir, dir[0].ToString());
                    destChildDir[1] = dstDir;
                    destChildDir[2] = dir[2];

                    directories.Rows.Add(destChildDir);
                }

                DuplicateFolder(duplicateFiles, components, files, directories, createFolders, sourceLineNumber, dir[0].ToString(), destChildDir[0].ToString());
            }
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
