using PanelSw.Wix.Extensions.Symbols;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using WixToolset.Data;
using WixToolset.Data.Symbols;
using WixToolset.Data.WindowsInstaller;
using WixToolset.Extensibility;

namespace PanelSw.Wix.Extensions
{
    public class PanelSwWiBackendBinder : BaseWindowsInstallerBackendBinderExtension
    {

        #region Assign section ID
        public override void FinalizePatchFilterIds(WindowsInstallerData data, IDictionary<Row, string> rowToFilterId, string filterIdPrefix)
        {
            base.FinalizePatchFilterIds(data, rowToFilterId, filterIdPrefix);
            GenerateSectionIds(data, rowToFilterId, filterIdPrefix);
        }

        // Assigning section ID informs WiX which rows belong to a patch
        // Rows are included in a patch in any of these cases:
        //   1. They have been explictly ref'ed by it (i.e. CustomPatchRef)
        //   2. They were implictly ref'ed by it (have same SectionId as another element that was ref'ed in the patch)
        //      For this case we attempt to assign section ID of a foreign row where possible
        //TODO Known Issues
        // 1. When an implicit PSW_xxx table is referenced is a patch, the related custom action isn't references. 
        //    Thus, if v1 had no RegularExpression, and v2 does have, then referencing the search with PropertyRef will not include the custom action, but will include the PSW_RegularExpression table
        //    Referencing the PSW_RegularExpression using a "CustomPatchRef" solves it
        // 2. When an DrLocator entry is reused by several AppSearch entries, it's SectionId is set by only one of the searches
        public void GenerateSectionIds(WindowsInstallerData data, IDictionary<Row, string> rowToFilterId, string filterIdPrefix)
        {
            Table componentTable = data.Tables["Component"];
            Table fileTable = data.Tables["File"];
            Table propertyTable = data.Tables["Property"];
            Table binaryTable = data.Tables["Binary"];

            Dictionary<string, List<ForeignRelation>> tableForeignKeys = new Dictionary<string, List<ForeignRelation>>();
            List<string> delayedTables = new List<string>();
            delayedTables.Add("PSW_ExecOnComponent_ExitCode");
            delayedTables.Add("PSW_ExecOn_ConsoleOutput");
            delayedTables.Add("PSW_ExecOnComponent_Environment");
            delayedTables.Add("PSW_ServiceConfig_Dependency");

            IncludeWixTables(data, tableForeignKeys);

            // Include internal tables except those that will be resolved later
            foreach (Table t in data.Tables)
            {
                if (!tableForeignKeys.ContainsKey(t.Definition.Name) && !delayedTables.Contains(t.Definition.Name) && !t.Definition.Unreal)
                {
                    tableForeignKeys[t.Definition.Name] = new List<ForeignRelation>();
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
            tableForeignKeys["PSW_ConcatFiles"].Add(new ForeignRelation(0, componentTable, 0));
            tableForeignKeys["PSW_ConcatFiles"].Add(new ForeignRelation(1, fileTable, 0));
            tableForeignKeys["PSW_ConcatFiles"].Add(new ForeignRelation(2, fileTable, 0));
            tableForeignKeys["PSW_Payload"].Add(new ForeignRelation(0, binaryTable, 0));

            AssignSectionIdToTables(data, tableForeignKeys, rowToFilterId, filterIdPrefix);

            // Add foreign relations for internal tables that reference other internal tables
            tableForeignKeys.Clear();
            foreach (string table in delayedTables)
            {
                tableForeignKeys[table] = new List<ForeignRelation>();
            }

            Table execOnTable = data.Tables["PSW_ExecOnComponent"];
            Table serviceConfigTable = data.Tables["PSW_ServiceConfig"];
            tableForeignKeys["PSW_ExecOnComponent_ExitCode"].Add(new ForeignRelation(0, execOnTable, 0));
            tableForeignKeys["PSW_ExecOn_ConsoleOutput"].Add(new ForeignRelation(1, execOnTable, 0));
            tableForeignKeys["PSW_ExecOnComponent_Environment"].Add(new ForeignRelation(0, execOnTable, 0));
            tableForeignKeys["PSW_ServiceConfig_Dependency"].Add(new ForeignRelation(0, serviceConfigTable, 0));
            AssignSectionIdToTables(data, tableForeignKeys, rowToFilterId, filterIdPrefix);

            ResolveAppSearch(data, rowToFilterId);
        }

        // Include some WiX/MSI tables
        private void IncludeWixTables(WindowsInstallerData data, Dictionary<string, List<ForeignRelation>> tableForeignKeys)
        {
            Table binaryTable = data.Tables["Binary"];
            Table fileTable = data.Tables["File"];
            Table propertyTable = data.Tables["Property"];
            Table componentTable = data.Tables["Component"];
            Table directoryTable = data.Tables["Directory"];
            Table userTable = data.Tables["Wix4User"];
            Table fileShareTable = data.Tables["Wix4FileShare"];
            Table groupTable = data.Tables["Wix4Group"];
            Table xmlConfigTable = data.Tables["Wix4XmlConfig"];

            // Windows Installer tables
            tableForeignKeys["AppSearch"] = new List<ForeignRelation>();
            tableForeignKeys["AppSearch"].Add(new ForeignRelation(0, propertyTable, 0));

            tableForeignKeys["MsiServiceConfig"] = new List<ForeignRelation>();
            tableForeignKeys["MsiServiceConfig"].Add(new ForeignRelation(5, componentTable, 0));

            // WixUtilExtension
            tableForeignKeys["Wix4CloseApplication"] = new List<ForeignRelation>();

            tableForeignKeys["Wix4RemoveFolderEx"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4RemoveFolderEx"].Add(new ForeignRelation(1, componentTable, 0));

            tableForeignKeys["Wix4RemoveRegistryKeyEx"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4RemoveRegistryKeyEx"].Add(new ForeignRelation(1, componentTable, 0));

            tableForeignKeys["Wix4RestartResource"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4RestartResource"].Add(new ForeignRelation(1, componentTable, 0));

            tableForeignKeys["Wix4FileShare"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4FileShare"].Add(new ForeignRelation(2, componentTable, 0));
            tableForeignKeys["Wix4FileShare"].Add(new ForeignRelation(4, directoryTable, 0));

            tableForeignKeys["Wix4FileSharePermissions"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4FileSharePermissions"].Add(new ForeignRelation(0, fileShareTable, 0));
            tableForeignKeys["Wix4FileSharePermissions"].Add(new ForeignRelation(1, userTable, 0));

            tableForeignKeys["Wix4Group"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4Group"].Add(new ForeignRelation(1, componentTable, 0));

            tableForeignKeys["Wix4InternetShortcut"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4InternetShortcut"].Add(new ForeignRelation(1, componentTable, 0));
            tableForeignKeys["Wix4InternetShortcut"].Add(new ForeignRelation(2, directoryTable, 0));

            tableForeignKeys["Wix4PerformanceCategory"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4PerformanceCategory"].Add(new ForeignRelation(1, componentTable, 0));

            tableForeignKeys["Wix4Perfmon"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4Perfmon"].Add(new ForeignRelation(0, componentTable, 0));

            tableForeignKeys["Wix4PerfmonManifest"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4PerfmonManifest"].Add(new ForeignRelation(0, componentTable, 0));

            tableForeignKeys["Wix4EventManifest"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4EventManifest"].Add(new ForeignRelation(0, componentTable, 0));

            tableForeignKeys["Wix4SecureObject"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4SecureObject"].Add(new ForeignRelation(6, componentTable, 0));

            tableForeignKeys["Wix4ServiceConfig"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4ServiceConfig"].Add(new ForeignRelation(1, componentTable, 0));

            tableForeignKeys["Wix4TouchFile"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4TouchFile"].Add(new ForeignRelation(1, componentTable, 0));

            tableForeignKeys["Wix4User"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4User"].Add(new ForeignRelation(1, componentTable, 0));

            tableForeignKeys["Wix4UserGroup"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4UserGroup"].Add(new ForeignRelation(0, userTable, 0));
            tableForeignKeys["Wix4UserGroup"].Add(new ForeignRelation(1, groupTable, 0));

            tableForeignKeys["Wix4XmlFile"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4XmlFile"].Add(new ForeignRelation(6, componentTable, 0));

            tableForeignKeys["Wix4XmlConfig"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4XmlConfig"].Add(new ForeignRelation(2, xmlConfigTable, 0));
            tableForeignKeys["Wix4XmlConfig"].Add(new ForeignRelation(8, componentTable, 0));

            tableForeignKeys["Wix4FormatFile"] = new List<ForeignRelation>();
            tableForeignKeys["Wix4FormatFile"].Add(new ForeignRelation(0, binaryTable, 0));
            tableForeignKeys["Wix4FormatFile"].Add(new ForeignRelation(1, fileTable, 0));
        }

        private void ResolveAppSearch(WindowsInstallerData data, IDictionary<Row, string> rowToFilterId)
        {
            Table appSearchTable = data.Tables["AppSearch"];
            if (appSearchTable != null)
            {
                foreach (Row appSearchRow in appSearchTable.Rows)
                {
                    ResolveAppSearchTree(data, appSearchRow, rowToFilterId);
                }
            }
        }

        // Locator Types
        private void ResolveAppSearchTree(WindowsInstallerData data, Row appSearchRow, IDictionary<Row, string> rowToFilterId)
        {
            Table signatureTable = data.Tables["Signature"];
            string key = appSearchRow.Fields[1].Data as string;
            if (signatureTable != null)
            {
                foreach (Row signatureRow in signatureTable.Rows)
                {
                    if (key.Equals(signatureRow.Fields[0].Data) && !rowToFilterId.ContainsKey(signatureRow) && rowToFilterId.ContainsKey(appSearchRow))
                    {
                        rowToFilterId[signatureRow] = rowToFilterId[appSearchRow];
                    }
                }
            }

            Table regLocatorTable = data.Tables["RegLocator"];
            if (regLocatorTable != null)
            {
                foreach (Row regRow in regLocatorTable.Rows)
                {
                    if (key.Equals(regRow.Fields[0].Data) && !rowToFilterId.ContainsKey(regRow) && rowToFilterId.ContainsKey(appSearchRow))
                    {
                        rowToFilterId[regRow] = rowToFilterId[appSearchRow];
                        ResolveRegLocatorTree(data, regRow, rowToFilterId);
                    }
                }
            }

            Table compLocatorTable = data.Tables["CompLocator"];
            if (compLocatorTable != null)
            {
                foreach (Row compRow in compLocatorTable.Rows)
                {
                    if (key.Equals(compRow.Fields[0].Data) && !rowToFilterId.ContainsKey(compRow) && rowToFilterId.ContainsKey(appSearchRow))
                    {
                        rowToFilterId[compRow] = rowToFilterId[appSearchRow];
                        ResolveCompLocatorTree(data, compRow, rowToFilterId);
                    }
                }
            }

            Table drLocatorTable = data.Tables["DrLocator"];
            if (drLocatorTable != null)
            {
                foreach (Row drRow in drLocatorTable.Rows)
                {
                    if (key.Equals(drRow.Fields[0].Data) && !rowToFilterId.ContainsKey(drRow) && rowToFilterId.ContainsKey(appSearchRow))
                    {
                        rowToFilterId[drRow] = rowToFilterId[appSearchRow];
                        ResolveDrLocatorTree(data, drRow, rowToFilterId);
                    }
                }
            }

            Table iniLocatorTable = data.Tables["IniLocator"];
            if (iniLocatorTable != null)
            {
                foreach (Row iniRow in iniLocatorTable.Rows)
                {
                    if (key.Equals(iniRow.Fields[0].Data) && !rowToFilterId.ContainsKey(iniRow) && rowToFilterId.ContainsKey(appSearchRow))
                    {
                        rowToFilterId[iniRow] = rowToFilterId[appSearchRow];
                        ResolveIniLocatorTree(data, iniRow, rowToFilterId);
                    }
                }
            }
        }

        private void ResolveIniLocatorTree(WindowsInstallerData data, Row iniRow, IDictionary<Row, string> rowToFilterId)
        {
            string key = iniRow.Fields[0]?.Data as string;

            Table signatureTable = data.Tables["Signature"];
            if (signatureTable != null)
            {
                foreach (Row signatureRow in signatureTable.Rows)
                {
                    if (key.Equals(signatureRow.Fields[0].Data) && !rowToFilterId.ContainsKey(signatureRow) && rowToFilterId.ContainsKey(iniRow))
                    {
                        rowToFilterId[signatureRow] = rowToFilterId[iniRow];
                    }
                }
            }
        }

        private void ResolveCompLocatorTree(WindowsInstallerData data, Row compRow, IDictionary<Row, string> rowToFilterId)
        {
            string key = compRow.Fields[0]?.Data as string;
            // File?
            Table signatureTable = data.Tables["Signature"];
            if (signatureTable != null)
            {
                foreach (Row signatureRow in signatureTable.Rows)
                {
                    if (key.Equals(signatureRow.Fields[0].Data) && !rowToFilterId.ContainsKey(signatureRow) && rowToFilterId.ContainsKey(compRow))
                    {
                        rowToFilterId[signatureRow] = rowToFilterId[compRow];
                    }
                }
            }
        }

        private void ResolveRegLocatorTree(WindowsInstallerData data, Row regRow, IDictionary<Row, string> rowToFilterId)
        {
            string key = regRow.Fields[0]?.Data as string;

            // File?
            Table signatureTable = data.Tables["Signature"];
            if (signatureTable != null)
            {
                foreach (Row signatureRow in signatureTable.Rows)
                {
                    if (key.Equals(signatureRow.Fields[0].Data) && !rowToFilterId.ContainsKey(signatureRow) && rowToFilterId.ContainsKey(regRow))
                    {
                        rowToFilterId[signatureRow] = rowToFilterId[regRow];
                    }
                }
            }
        }

        private void ResolveDrLocatorTree(WindowsInstallerData data, Row drRow, IDictionary<Row, string> rowToFilterId)
        {
            string key = drRow.Fields[0].Data as string;

            // File?
            Table signatureTable = data.Tables["Signature"];
            if (signatureTable != null)
            {
                foreach (Row signatureRow in signatureTable.Rows)
                {
                    if (key.Equals(signatureRow.Fields[0].Data) && !rowToFilterId.ContainsKey(signatureRow) && rowToFilterId.ContainsKey(drRow))
                    {
                        rowToFilterId[signatureRow] = rowToFilterId[drRow];
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

                    if (parent.Equals(otherDrRow.Fields[0].Data) && !rowToFilterId.ContainsKey(otherDrRow) && rowToFilterId.ContainsKey(drRow))
                    {
                        rowToFilterId[otherDrRow] = rowToFilterId[drRow];
                        ResolveDrLocatorTree(data, otherDrRow, rowToFilterId);
                    }
                }
                Table regLocatorTable = data.Tables["RegLocator"];
                if (regLocatorTable != null)
                {
                    foreach (Row regRow in regLocatorTable.Rows)
                    {
                        if (parent.Equals(regRow.Fields[0].Data) && !rowToFilterId.ContainsKey(regRow) && rowToFilterId.ContainsKey(drRow))
                        {
                            rowToFilterId[regRow] = rowToFilterId[drRow];
                            ResolveRegLocatorTree(data, regRow, rowToFilterId);
                        }
                    }
                }

                Table compLocatorTable = data.Tables["CompLocator"];
                if (compLocatorTable != null)
                {
                    foreach (Row compRow in compLocatorTable.Rows)
                    {
                        if (parent.Equals(compRow.Fields[0].Data) && !rowToFilterId.ContainsKey(compRow) && rowToFilterId.ContainsKey(drRow))
                        {
                            rowToFilterId[compRow] = rowToFilterId[drRow];
                            ResolveCompLocatorTree(data, compRow, rowToFilterId);
                        }
                    }
                }

                Table iniLocatorTable = data.Tables["IniLocator"];
                if (iniLocatorTable != null)
                {
                    foreach (Row iniRow in iniLocatorTable.Rows)
                    {
                        if (parent.Equals(iniRow.Fields[0].Data) && !rowToFilterId.ContainsKey(iniRow) && rowToFilterId.ContainsKey(drRow))
                        {
                            rowToFilterId[iniRow] = rowToFilterId[drRow];
                            ResolveIniLocatorTree(data, iniRow, rowToFilterId);
                        }
                    }
                }
            }
        }

        private uint sectionId_ = 0;
        private void AssignSectionIdToTables(WindowsInstallerData data, Dictionary<string, List<ForeignRelation>> tableForeignKeys, IDictionary<Row, string> rowToFilterId, string filterIdPrefix)
        {
            foreach (Table t in data.Tables)
            {
                if (tableForeignKeys.ContainsKey(t.Name))
                {
                    foreach (Row r in t.Rows)
                    {
                        if (rowToFilterId.ContainsKey(r))
                        {
                            continue;
                        }

                        // Prefer foreign section
                        foreach (ForeignRelation foreignRelation in tableForeignKeys[t.Name])
                        {
                            if (AssignForeignSectionId(r, foreignRelation, rowToFilterId))
                            {
                                break;
                            }
                        }
                        if (!rowToFilterId.ContainsKey(r))
                        {
                            rowToFilterId[r] = $"{filterIdPrefix}psw.section.{++sectionId_}";
                        }
                    }
                }
            }
        }

        private bool AssignForeignSectionId(Row targetRow, ForeignRelation foreignRelation, IDictionary<Row, string> rowToFilterId)
        {
            if ((foreignRelation.ForeignTable == null)
                || (targetRow.Fields[foreignRelation.LocalKeyIndex] == null)
                || (targetRow.Fields[foreignRelation.LocalKeyIndex].Data == null))
            {
                return false;
            }

            foreach (Row foreignRow in foreignRelation.ForeignTable.Rows)
            {
                if (rowToFilterId.ContainsKey(foreignRow)
                    && (foreignRow.Fields[foreignRelation.ForeignKeyIndex] != null)
                    && targetRow.Fields[foreignRelation.LocalKeyIndex].Data.Equals(foreignRow.Fields[foreignRelation.ForeignKeyIndex].Data))
                {
                    rowToFilterId[targetRow] = rowToFilterId[foreignRow];
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

        #endregion

        #region Table definitions

        private List<TableDefinition> _tableDefinition;
        public override IReadOnlyCollection<TableDefinition> TableDefinitions
        {
            get
            {
                if (_tableDefinition == null)
                {
                    _tableDefinition = new List<TableDefinition>
                    {
                        new TableDefinition(nameof(PSW_AccountSidSearch), PSW_AccountSidSearch.SymbolDefinition, PSW_AccountSidSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_BackupAndRestore), PSW_BackupAndRestore.SymbolDefinition, PSW_BackupAndRestore.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_CertificateHashSearch), PSW_CertificateHashSearch.SymbolDefinition, PSW_CertificateHashSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ConcatFiles), PSW_ConcatFiles.SymbolDefinition, PSW_ConcatFiles.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_CustomUninstallKey), PSW_CustomUninstallKey.SymbolDefinition, PSW_CustomUninstallKey.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_DeletePath), PSW_DeletePath.SymbolDefinition, PSW_DeletePath.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_DiskSpace), PSW_DiskSpace.SymbolDefinition, PSW_DiskSpace.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_Dism), PSW_Dism.SymbolDefinition, PSW_Dism.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_EvaluateExpression), PSW_EvaluateExpression.SymbolDefinition, PSW_EvaluateExpression.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ExecOn_ConsoleOutput), PSW_ExecOn_ConsoleOutput.SymbolDefinition, PSW_ExecOn_ConsoleOutput.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ExecOnComponent), PSW_ExecOnComponent.SymbolDefinition, PSW_ExecOnComponent.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ExecOnComponent_Environment), PSW_ExecOnComponent_Environment.SymbolDefinition, PSW_ExecOnComponent_Environment.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_ExecOnComponent_ExitCode), PSW_ExecOnComponent_ExitCode.SymbolDefinition, PSW_ExecOnComponent_ExitCode.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_FileRegex), PSW_FileRegex.SymbolDefinition, PSW_FileRegex.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ForceVersion), PSW_ForceVersion.SymbolDefinition, PSW_ForceVersion.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_InstallUtil), PSW_InstallUtil.SymbolDefinition, PSW_InstallUtil.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_InstallUtil_Arg), PSW_InstallUtil_Arg.SymbolDefinition, PSW_InstallUtil_Arg.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_IsWindowsVersionOrGreater), PSW_IsWindowsVersionOrGreater.SymbolDefinition, PSW_IsWindowsVersionOrGreater.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_JsonJPath), PSW_JsonJPath.SymbolDefinition, PSW_JsonJPath.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_JsonJpathSearch), PSW_JsonJpathSearch.SymbolDefinition, PSW_JsonJpathSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_Md5Hash), PSW_Md5Hash.SymbolDefinition, PSW_Md5Hash.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_MsiSqlQuery), PSW_MsiSqlQuery.SymbolDefinition, PSW_MsiSqlQuery.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_PathSearch), PSW_PathSearch.SymbolDefinition, PSW_PathSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_Payload), PSW_Payload.SymbolDefinition, PSW_Payload.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_ReadIniValues), PSW_ReadIniValues.SymbolDefinition, PSW_ReadIniValues.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_RegularExpression), PSW_RegularExpression.SymbolDefinition, PSW_RegularExpression.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_RemoveRegistryValue), PSW_RemoveRegistryValue.SymbolDefinition, PSW_RemoveRegistryValue.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_RestartLocalResources), PSW_RestartLocalResources.SymbolDefinition, PSW_RestartLocalResources.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_SelfSignCertificate), PSW_SelfSignCertificate.SymbolDefinition, PSW_SelfSignCertificate.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ServiceConfig), PSW_ServiceConfig.SymbolDefinition, PSW_ServiceConfig.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ServiceConfig_Dependency), PSW_ServiceConfig_Dependency.SymbolDefinition, PSW_ServiceConfig_Dependency.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_SetPropertyFromPipe), PSW_SetPropertyFromPipe.SymbolDefinition, PSW_SetPropertyFromPipe.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ShellExecute), PSW_ShellExecute.SymbolDefinition, PSW_ShellExecute.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_SqlScript), PSW_SqlScript.SymbolDefinition, PSW_SqlScript.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_SqlScript_Replacements), PSW_SqlScript_Replacements.SymbolDefinition, PSW_SqlScript_Replacements.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_SqlSearch), PSW_SqlSearch.SymbolDefinition, PSW_SqlSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_TaskScheduler), PSW_TaskScheduler.SymbolDefinition, PSW_TaskScheduler.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_Telemetry), PSW_Telemetry.SymbolDefinition, PSW_Telemetry.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ToLowerCase), PSW_ToLowerCase.SymbolDefinition, PSW_ToLowerCase.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_TopShelf), PSW_TopShelf.SymbolDefinition, PSW_TopShelf.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_Unzip), PSW_Unzip.SymbolDefinition, PSW_Unzip.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_VersionCompare), PSW_VersionCompare.SymbolDefinition, PSW_VersionCompare.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_WebsiteConfig), PSW_WebsiteConfig.SymbolDefinition, PSW_WebsiteConfig.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_WmiSearch), PSW_WmiSearch.SymbolDefinition, PSW_WmiSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_XmlSearch), PSW_XmlSearch.SymbolDefinition, PSW_XmlSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_XslTransform), PSW_XslTransform.SymbolDefinition, PSW_XslTransform.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_XslTransform_Replacements), PSW_XslTransform_Replacements.SymbolDefinition, PSW_XslTransform_Replacements.ColumnDefinitions, symbolIdIsPrimaryKey: false),
                        new TableDefinition(nameof(PSW_ZipFile), PSW_ZipFile.SymbolDefinition, PSW_ZipFile.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                    };
                }

                return _tableDefinition;
            }
        }

        #endregion

        #region Bind split files

        private List<string> tempFiles_ = new List<string>();
        ~PanelSwWiBackendBinder()
        {
            // Delete temporary files
            foreach (string f in tempFiles_)
            {
                File.Delete(f);
            }
        }

        public override void SymbolsFinalized(IntermediateSection section)
        {
            base.SymbolsFinalized(section);
            GetSplitFiles(section);
            ResolveTaskScheduler(section);
        }

        private void GetSplitFiles(IntermediateSection section)
        {
            // This section is empty, need to iterate the other sections
            List<PSW_ConcatFiles> concatFiles = new List<PSW_ConcatFiles>();
            List<FileSymbol> allFiles = new List<FileSymbol>();
            foreach (IntermediateSection intermediate in base.Context.IntermediateRepresentation.Sections)
            {
                foreach (IntermediateSymbol symbol in intermediate.Symbols)
                {
                    if (symbol is PSW_ConcatFiles concatSymbol)
                    {
                        concatFiles.Add(concatSymbol);
                    }
                    else if (symbol is FileSymbol f)
                    {
                        allFiles.Add(f);
                    }
                }
            }

            // Collect temporary file paths to later delete
            foreach (PSW_ConcatFiles concat in concatFiles)
            {
                FileSymbol fileSymbol = allFiles.FirstOrDefault(f => f.Id.Id.Equals(concat.RootFile_));
                if ((fileSymbol != null) && !tempFiles_.Contains(fileSymbol.Source.Path))
                {
                    tempFiles_.Add(fileSymbol.Source.Path);
                }
                fileSymbol = allFiles.FirstOrDefault(f => f.Id.Id.Equals(concat.MyFile_));
                if ((fileSymbol != null) && !tempFiles_.Contains(fileSymbol.Source.Path))
                {
                    tempFiles_.Add(fileSymbol.Source.Path);
                }
            }
        }

        #endregion

        private void ResolveTaskScheduler(IntermediateSection section)
        {
            List<PSW_TaskScheduler> tasks = new List<PSW_TaskScheduler>();
            foreach (IntermediateSection intermediate in base.Context.IntermediateRepresentation.Sections)
            {
                foreach (IntermediateSymbol symbol in intermediate.Symbols)
                {
                    if (symbol is PSW_TaskScheduler taskSymbol)
                    {
                        tasks.Add(taskSymbol);
                    }
                }
            }

            foreach (PSW_TaskScheduler r in tasks)
            {
                if (r.TaskXml.Contains("!(bindpath."))
                {
                    Messaging.Write(ErrorMessages.UnresolvedBindReference(null, $"TaskScheduler XmlFile {r.TaskXml}"));
                }
                if (!File.Exists(r.TaskXml))
                {
                    continue;
                }
                string xml = File.ReadAllText(r.TaskXml);
                xml = xml.Trim();
                xml = xml.Replace("\r", "");
                xml = xml.Replace("\n", "");
                xml = xml.Replace(Environment.NewLine, "");
                r.TaskXml = xml;
            }
        }
    }
}
