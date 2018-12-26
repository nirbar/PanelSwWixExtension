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

        private void AlwaysOverwriteFiles(Output output)
        {
            List<string> overwriteFiles = new List<string>();

            Table overwriteT = output.Tables["PSW_AlwaysOverwriteFile"];
            if (overwriteT == null)
            {
                return;
            }

            foreach (Row r in overwriteT.Rows)
            {
                overwriteFiles.Add(r.Fields[0].Data.ToString());
            }

            Table fileT = output.Tables["File"];
            if (fileT == null)
            {
                return;
            }
            foreach (Row r in fileT.Rows)
            {
                string id = r.Fields[0].Data.ToString();
                if (overwriteFiles.Contains(id))
                {
                    foreach (Field f in r.Fields)
                    {
                        if (f.Column.Name.Equals("Version"))
                        {
                            f.Data = "65535.65535.65535.65535";
                        }
                        if (f.Column.Name.Equals("Attributes")) // Remove file from MsiFileHash table, ICE60
                        {
                            int attr = (int)f.Data;
                            attr &= ~0x000200; //msidbFileAttributesChecksum
                            f.Data = attr;
                        }
                    }
                }
            }

            // Remove file from MsiFileHash table, ICE60
            Table hashT = output.Tables["MsiFileHash"];
            if (hashT == null)
            {
                return;
            }
            for (int i = hashT.Rows.Count - 1; i >= 0; --i)
            {
                string id = hashT.Rows[i].Fields[0].Data.ToString();
                if (overwriteFiles.Contains(id))
                {
                    hashT.Rows.RemoveAt(i);
                }
            }
        }
    }
}
