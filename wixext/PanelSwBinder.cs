using Microsoft.Tools.WindowsInstallerXml;
using System.Collections.Generic;
using System;

namespace PanelSw.Wix.Extensions
{
    class PanelSwBinder : BinderExtensionEx
    {
        public override void DatabaseFinalize(Output output)
        {
            if (output.Type == OutputType.Product)
            {
                AlwaysOverwriteFiles(output);
            }
        }

        private int ColumnByName(Table tbl, string name)
        {
            for (int i = 0; i < tbl.Definition.Columns.Count; ++i)
            {
                ColumnDefinition col = tbl.Definition.Columns[i];
                if (col.Name.Equals(name))
                {
                    return i;
                }
            }
            throw new KeyNotFoundException($"Did not find column '{name}' in table '{tbl.Name}'");
        }

        private Row RowByKey(Table tbl, int keyCol, string keyVal)
        {
            foreach (Row r in tbl.Rows)
            {
                if (r[keyCol].Equals(keyVal))
                {
                    return r;
                }
            }
            return null;
        }

        private int RowIndexByKey(Table tbl, int keyCol, string keyVal)
        {
            for (int i = tbl.Rows.Count - 1; i >= 0; --i)
            {
                if (tbl.Rows[i][keyCol].Equals(keyVal))
                {
                    return i;
                }
            }
            return -1;
        }

        private void AlwaysOverwriteFiles(Output output)
        {
            Table overwriteT = output.Tables["PSW_AlwaysOverwriteFile"];
            if (overwriteT == null)
            {
                return;
            }

            Table fileT = output.Tables["File"];
            if (fileT == null)
            {
                return;
            }

            int fileKeyCol = ColumnByName(fileT, "File");
            int fileVersionCol = ColumnByName(fileT, "Version");
            int fileLanguageCol = ColumnByName(fileT, "Language");

            Table hashT = output.Tables["MsiFileHash"];

            foreach (Row overR in overwriteT.Rows)
            {
                string srcLineStr = overR[1]?.ToString();
                SourceLineNumberCollection srcLines = (srcLineStr == null) ? new SourceLineNumberCollection("") : new SourceLineNumberCollection(srcLineStr);
                string fileId = overR[0].ToString();
                if (string.IsNullOrEmpty(fileId))
                {
                    Core.OnMessage(WixErrors.IdentifierNotFound("AlwaysOverwriteFile", ""));
                    continue;
                }
                Row fileR = RowByKey(fileT, fileKeyCol, fileId);
                if (fileR == null)
                {
                    Core.OnMessage(WixErrors.FileIdentifierNotFound(srcLines, fileId));
                    continue;
                }

                fileR[fileVersionCol] = "65535.65535.65535.65535";

                // Remove file from MsiFileHash table, ICE60
                if (hashT != null)
                {
                    int hashKeyCol = ColumnByName(hashT, "File_");
                    int hashRow = RowIndexByKey(hashT, hashKeyCol, fileId);
                    if (hashRow >= 0)
                    {
                        hashT.Rows.RemoveAt(hashRow);
                    }
                }

                // Language
                object lang = fileR[fileLanguageCol];
                if (string.IsNullOrEmpty(lang?.ToString()))
                {
                    fileR[fileLanguageCol] = "0";
                }
            }

            if ((hashT != null) && (hashT.Rows.Count == 0))
            {
                output.Tables.Remove(hashT.Name);
            }
        }
    }
}
