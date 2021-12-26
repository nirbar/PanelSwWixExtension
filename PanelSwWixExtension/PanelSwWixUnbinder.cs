﻿using Microsoft.Tools.WindowsInstallerXml;
using System;
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
            
            IncludeWixTables(output, tableForeignKeys);

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

            ResolveAppSearch(output);
        }

        // Include some WiX/MSI tables
        private void IncludeWixTables(Output output, Dictionary<string, List<ForeignRelation>> tableForeignKeys)
        {
            Table propertyTable = output.Tables["Property"];
          
            tableForeignKeys["AppSearch"] = new List<ForeignRelation>();
            tableForeignKeys["AppSearch"].Add(new ForeignRelation(0, propertyTable, 0));
        }

        private void ResolveAppSearch(Output output)
        {
            Table appSearchTable = output.Tables["AppSearch"];
            if (appSearchTable != null)
            {
                foreach (Row appSearchRow in appSearchTable.Rows)
                {
                    ResolveAppSearchTree(output, appSearchRow);
                }
            }
        }

        // Locator Types
        private void ResolveAppSearchTree(Output output, Row appSearchRow)
        {
            Table signatureTable = output.Tables["Signature"];
            string key = appSearchRow.Fields[1].Data as string;
            if (signatureTable != null)
            {
                foreach (Row signatureRow in signatureTable.Rows)
                {
                    if (key.Equals(signatureRow.Fields[0].Data))
                    {
                        signatureRow.SectionId = appSearchRow.SectionId;
                    }
                }
            }

            Table regLocatorTable = output.Tables["RegLocator"];
            if (regLocatorTable != null)
            {
                foreach (Row regRow in regLocatorTable.Rows)
                {
                    if (key.Equals(regRow.Fields[0].Data))
                    {
                        regRow.SectionId = appSearchRow.SectionId;
                        ResolveRegLocatorTree(output, regRow);
                    }
                }
            }

            Table compLocatorTable = output.Tables["CompLocator"];
            if (compLocatorTable != null)
            {
                foreach (Row compRow in compLocatorTable.Rows)
                {
                    if (key.Equals(compRow.Fields[0].Data))
                    {
                        compRow.SectionId = appSearchRow.SectionId;
                        ResolveCompLocatorTree(output, compRow);
                    }
                }
            }

            Table drLocatorTable = output.Tables["DrLocator"];
            if (drLocatorTable != null)
            {
                foreach (Row drRow in drLocatorTable.Rows)
                {
                    if (key.Equals(drRow.Fields[0].Data))
                    {
                        drRow.SectionId = appSearchRow.SectionId;
                        ResolveDrLocatorTree(output, drRow);
                    }
                }
            }

            Table iniLocatorTable = output.Tables["IniLocator"];
            if (iniLocatorTable != null)
            {
                foreach (Row iniRow in iniLocatorTable.Rows)
                {
                    if (key.Equals(iniRow.Fields[0].Data))
                    {
                        iniRow.SectionId = appSearchRow.SectionId;
                        ResolveIniLocatorTree(output, iniRow);
                    }
                }
            }
        }

        private void ResolveIniLocatorTree(Output output, Row iniRow)
        {
            string key = iniRow.Fields[0]?.Data as string;

            Table signatureTable = output.Tables["Signature"];
            if (signatureTable != null)
            {
                foreach (Row signatureRow in signatureTable.Rows)
                {
                    if (key.Equals(signatureRow.Fields[0].Data))
                    {
                        signatureRow.SectionId = iniRow.SectionId;
                    }
                }
            }
        }

        private void ResolveCompLocatorTree(Output output, Row compRow)
        {
            string key = compRow.Fields[0]?.Data as string;
            // File?
            Table signatureTable = output.Tables["Signature"];
            if (signatureTable != null)
            {
                foreach (Row signatureRow in signatureTable.Rows)
                {
                    if (key.Equals(signatureRow.Fields[0].Data))
                    {
                        signatureRow.SectionId = compRow.SectionId;
                    }
                }
            }
        }

        private void ResolveRegLocatorTree(Output output, Row regRow)
        {
            string key = regRow.Fields[0]?.Data as string;

            // File?
            Table signatureTable = output.Tables["Signature"];
            if (signatureTable != null)
            {
                foreach (Row signatureRow in signatureTable.Rows)
                {
                    if (key.Equals(signatureRow.Fields[0].Data))
                    {
                        signatureRow.SectionId = regRow.SectionId;
                    }
                }
            }
        }

        private void ResolveDrLocatorTree(Output output, Row drRow)
        {
            string key = drRow.Fields[0].Data as string;
         
            // File?
            Table signatureTable = output.Tables["Signature"];
            if (signatureTable != null)
            {
                foreach (Row signatureRow in signatureTable.Rows)
                {
                    if (key.Equals(signatureRow.Fields[0].Data))
                    {
                        signatureRow.SectionId = drRow.SectionId;
                    }
                }
            }

            // Parent?
            string parent = drRow.Fields[1]?.Data as string;
            if (!string.IsNullOrEmpty(parent))
            {
                Table drLocatorTable = drRow.Table;
                foreach (Row otherDrRow in drLocatorTable.Rows)
                {
                    if (drRow.Equals(otherDrRow))
                    {
                        continue;
                    }

                    if (parent.Equals(otherDrRow.Fields[0].Data))
                    {
                        otherDrRow.SectionId = drRow.SectionId;
                        ResolveDrLocatorTree(output, otherDrRow);
                    }
                }
                Table regLocatorTable = output.Tables["RegLocator"];
                if (regLocatorTable != null)
                {
                    foreach (Row regRow in regLocatorTable.Rows)
                    {
                        if (parent.Equals(regRow.Fields[0].Data))
                        {
                            regRow.SectionId = drRow.SectionId;
                            ResolveRegLocatorTree(output, regRow);
                        }
                    }
                }

                Table compLocatorTable = output.Tables["CompLocator"];
                if (compLocatorTable != null)
                {
                    foreach (Row compRow in compLocatorTable.Rows)
                    {
                        if (parent.Equals(compRow.Fields[0].Data))
                        {
                            compRow.SectionId = drRow.SectionId;
                            ResolveCompLocatorTree(output, compRow);
                        }
                    }
                }

                Table iniLocatorTable = output.Tables["IniLocator"];
                if (iniLocatorTable != null)
                {
                    foreach (Row iniRow in iniLocatorTable.Rows)
                    {
                        if (parent.Equals(iniRow.Fields[0].Data))
                        {
                            iniRow.SectionId = drRow.SectionId;
                            ResolveIniLocatorTree(output, iniRow);
                        }
                    }
                }
            }
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