using Microsoft.Tools.WindowsInstallerXml;
using System.Collections.Generic;

namespace PanelSw.Wix.Extensions
{
    class PanelSwWixUnbinder : UnbinderExtension
    {
        TableDefinitionCollection tableDefinitions_;
        public PanelSwWixUnbinder(TableDefinitionCollection tableDefinitions)
        {
            tableDefinitions_ = tableDefinitions;
        }

        // Assigning section ID informs WiX which rows belong to a patch
        // Rows are included in a patch in any of these cases:
        //   1. They have been explictly ref'ed by it (i.e. CustomPatchRef)
        //   2. They were implictly ref'ed by it (have same SectionId as another element that was ref'ed in the patch)
        //      For this case we attempt to assign section ID of a foreign row where possible
        public override void GenerateSectionIds(Output output)
        {
            Table componentTable = output.Tables["Component"];
            Table fileTable = output.Tables["File"];
            Table propertyTable = output.Tables["Property"];

            Dictionary<string, List<ForeignRelation>> tableForeignKeys = new Dictionary<string, List<ForeignRelation>>();
            List<string> delayedTables = new List<string>();
            delayedTables.Add("PSW_ExecOnComponent_ExitCode");
            delayedTables.Add("PSW_ExecOn_ConsoleOutput");
            delayedTables.Add("PSW_ExecOnComponent_Environment");
            delayedTables.Add("PSW_ServiceConfig_Dependency");

            // Include internal tables except those that will be resolved later
            foreach (TableDefinition td in tableDefinitions_)
            {
                if (!td.IsUnreal && !td.IsBootstrapperApplicationData && !delayedTables.Contains(td.Name))
                {
                    tableForeignKeys[td.Name] = new List<ForeignRelation>();
                }
            }

            // Add foreign relations in order of preference
            tableForeignKeys["PSW_FileRegex"].Add(new ForeignRelation(2, fileTable, 0));
            tableForeignKeys["PSW_FileRegex"].Add(new ForeignRelation(1, componentTable, 0));

            tableForeignKeys["PSW_XmlSearch"].Add(new ForeignRelation(1, propertyTable, 0));
            tableForeignKeys["PSW_ReadIniValues"].Add(new ForeignRelation(4, propertyTable, 0));
            tableForeignKeys["PSW_RegularExpression"].Add(new ForeignRelation(5, propertyTable, 0));
            tableForeignKeys["PSW_MsiSqlQuery"].Add(new ForeignRelation(1, propertyTable, 0));
            tableForeignKeys["PSW_TaskScheduler"].Add(new ForeignRelation(2, componentTable, 0));
            tableForeignKeys["PSW_ExecOnComponent"].Add(new ForeignRelation(1, componentTable, 0));
            tableForeignKeys["PSW_Dism"].Add(new ForeignRelation(1, componentTable, 0));
            tableForeignKeys["PSW_ServiceConfig"].Add(new ForeignRelation(1, componentTable, 0));
            tableForeignKeys["PSW_InstallUtil"].Add(new ForeignRelation(0, fileTable, 0));
            tableForeignKeys["PSW_InstallUtil_Arg"].Add(new ForeignRelation(0, fileTable, 0));
            tableForeignKeys["PSW_SqlSearch"].Add(new ForeignRelation(1, propertyTable, 1));

            AssignSectionIdToTables(output, tableForeignKeys);

            // Add foreign relations for internal tables that reference other internal tables
            tableForeignKeys.Clear();
            foreach (string table in delayedTables)
            {
                tableForeignKeys[table] = new List<ForeignRelation>();
            }

            Table execOnTable = output.Tables["PSW_ExecOnComponent"];
            Table serviceConfigTable = output.Tables["PSW_ServiceConfig"];
            tableForeignKeys["PSW_ExecOnComponent_ExitCode"].Add(new ForeignRelation(0, execOnTable, 0));
            tableForeignKeys["PSW_ExecOn_ConsoleOutput"].Add(new ForeignRelation(1, execOnTable, 0));
            tableForeignKeys["PSW_ExecOnComponent_Environment"].Add(new ForeignRelation(0, execOnTable, 0));
            tableForeignKeys["PSW_ServiceConfig_Dependency"].Add(new ForeignRelation(0, serviceConfigTable, 0));
            AssignSectionIdToTables(output, tableForeignKeys);
        }

        private uint sectionId_ = 0;
        private void AssignSectionIdToTables(Output output, Dictionary<string, List<ForeignRelation>> tableForeignKeys)
        {
            foreach (Table t in output.Tables)
            {
                if (tableForeignKeys.ContainsKey(t.Name))
                {
                    foreach (Row r in t.Rows)
                    {
                        if (!string.IsNullOrEmpty(r.SectionId))
                        {
                            continue;
                        }

                        // Prefer foreign section
                        foreach (ForeignRelation foreignRelation in tableForeignKeys[t.Name])
                        {
                            if (AssignForeignSectionId(r, foreignRelation))
                            {
                                break;
                            }
                        }
                        if (string.IsNullOrEmpty(r.SectionId))
                        {
                            r.SectionId = $"psw.section.{++sectionId_}";
                        }
                    }
                }
            }
        }

        private bool AssignForeignSectionId(Row targetRow, ForeignRelation foreignRelation)
        {
            if ((foreignRelation.ForeignTable == null)
                || (targetRow.Fields[foreignRelation.LocalKeyIndex] == null)
                || (targetRow.Fields[foreignRelation.LocalKeyIndex].Data == null))
            {
                return false;
            }

            foreach (Row foreignRow in foreignRelation.ForeignTable.Rows)
            {
                if (!string.IsNullOrEmpty(foreignRow.SectionId)
                    && (foreignRow.Fields[foreignRelation.ForeignKeyIndex] != null)
                    && targetRow.Fields[foreignRelation.LocalKeyIndex].Data.Equals(foreignRow.Fields[foreignRelation.ForeignKeyIndex].Data))
                {
                    targetRow.SectionId = foreignRow.SectionId;
                    return true;
                }
            }
            return false;
        }

        struct ForeignRelation
        {
            public ForeignRelation(int localKeyIndex, Table foreignTable, int foreignKeyIndex)
            {
                LocalKeyIndex = localKeyIndex;
                ForeignTable = foreignTable;
                ForeignKeyIndex = foreignKeyIndex;
            }

            public int LocalKeyIndex;
            public Table ForeignTable;
            public int ForeignKeyIndex;
        }
    }
}