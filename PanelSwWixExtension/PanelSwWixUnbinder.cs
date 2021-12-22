using Microsoft.Tools.WindowsInstallerXml;

namespace PanelSw.Wix.Extensions
{
    class PanelSwWixUnbinder : UnbinderExtension
    {
        private bool AssignForeignSectionId(Row targetRow, int keyIndex, Table foreignTable, int foreignKeyIndex)
        {
            if ((foreignTable == null) || (targetRow.Fields[keyIndex] == null) || (targetRow.Fields[keyIndex].Data == null))
            {
                return false;
            }

            foreach (Row frgnRow in foreignTable.Rows)
            {
                if (!string.IsNullOrEmpty(frgnRow.SectionId) && (frgnRow.Fields[foreignKeyIndex] != null) && targetRow.Fields[keyIndex].Data.Equals(frgnRow.Fields[foreignKeyIndex].Data))
                {
                    targetRow.SectionId = frgnRow.SectionId;
                    return true;
                }
            }
            return false;
        }

        // Assigning section ID informs WiX which rows belong to a patch
        // Rows are included in a patch in any of these cases:
        //   1. They have been explictly ref'ed by it (i.e. CustomPatchRef)
        //   2. They were implictly ref'ed by it (have same SectionId as another element that was ref'ed in the patch)
        public override void GenerateSectionIds(Output output)
        {
            Table componentTable = output.Tables["Component"];
            Table fileTable = output.Tables["File"];
            Table propertyTable = output.Tables["Property"];

            foreach (Table t in output.Tables)
            {
                int i = 0;
                switch (t.Name)
                {
                    case "PSW_FileRegex":
                        foreach (Row r in t.Rows)
                        {
                            // File, Component, or self-assigned section
                            if (!AssignForeignSectionId(r, 2, fileTable, 0) && !AssignForeignSectionId(r, 1, componentTable, 0))
                            {
                                r.SectionId = $"psw.section.{++i}";
                            }
                        }
                        break;
                    case "PSW_XmlSearch":
                        foreach (Row r in t.Rows)
                        {
                            // Property, or self-assigned section
                            if (!AssignForeignSectionId(r, 1, propertyTable, 0))
                            {
                                r.SectionId = $"psw.section.{++i}";
                            }
                        }
                        break;

                        //TODO For each table, assign section. Prefer using section ID of a foreign row
                }
            }
        }
    }
}