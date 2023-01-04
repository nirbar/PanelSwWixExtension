using PanelSw.Wix.Extensions.Symbols;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml.Linq;
using WixToolset.Data;
using WixToolset.Data.Symbols;
using WixToolset.Data.WindowsInstaller;
using WixToolset.Extensibility;

namespace PanelSw.Wix.Extensions
{
    public class PanelSwWixDecompiler : BaseWindowsInstallerDecompilerExtension
    {
        public override IReadOnlyCollection<TableDefinition> TableDefinitions => PanelSwWixExtension.TableDefinitions;

        public override bool TryDecompileTable(Table table)
        {
            switch (table.Name)
            {
                case "PSW_AccountSidSearch":
                    this.DecompileAccountSidSearch(table);
                    break;
                case "PSW_BackupAndRestore":
                    this.DecompileBackupAndRestore(table);
                    break;
                default:
                    return false;
            }

            return true;
        }

        public override void PreDecompileTables(TableIndexedCollection tables)
        {
            if (tables.TryGetTable("Property", out Table propertyTable))
            {
                for (int i = propertyTable.Rows.Count - 1; i >= 0; --i)
                {
                    Row propRow = propertyTable.Rows[i];
                    string propName = propRow.FieldAsString(0);
                    if (propName.Equals("BackupAndRestore_deferred_After_DuplicateFiles")
                        || propName.Equals("BackupAndRestore_deferred_Before_InstallFiles")
                        || propName.Equals("BackupAndRestore_deferred_After_RemoveExistingProducts"))
                    {
                        propertyTable.Rows.RemoveAt(i);
                    }
                    else if (propName.Equals("MsiHiddenProperties"))
                    {
                        string propValue = propRow.FieldAsString(1);
                        if (!string.IsNullOrEmpty(propValue))
                        {
                            if (propValue.Contains("BackupAndRestore_deferred_After_DuplicateFiles"))
                            {
                                _backupAndRestoreSchedule = PanelSwWixCompiler.BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_After_DuplicateFiles;
                                propValue = propValue.Replace("BackupAndRestore_deferred_After_DuplicateFiles", "");
                            }
                            if (propValue.Contains("BackupAndRestore_deferred_Before_InstallFiles"))
                            {
                                _backupAndRestoreSchedule = PanelSwWixCompiler.BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_Before_InstallFiles;
                                propValue = propValue.Replace("BackupAndRestore_deferred_Before_InstallFiles", "");
                            }
                            if (propValue.Contains("BackupAndRestore_deferred_After_RemoveExistingProducts"))
                            {
                                _backupAndRestoreSchedule = PanelSwWixCompiler.BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_After_RemoveExistingProducts;
                                propValue = propValue.Replace("BackupAndRestore_deferred_After_RemoveExistingProducts", "");
                            }

                            propValue = propValue.Replace("BackupAndRestore_commit", "");
                            propValue = propValue.Replace("BackupAndRestore_deferred", "");
                            propValue = propValue.Replace("BackupAndRestore_rollback", "");
                            propValue = propValue.Replace("FileRegex_commit", "");
                            propValue = propValue.Replace("FileRegex_deferred", "");
                            propValue = propValue.Replace("FileRegex_rollback", "");
                            propValue = propValue.Trim(';');
                            propRow[1] = propValue;
                        }
                    }
                }
            }
        }

        private void DecompileAccountSidSearch(Table table)
        {
            foreach (var row in table.Rows)
            {
                string propName = row.FieldAsString(1);
                XElement xProperty = null;
                if (!this.DecompilerHelper.TryGetIndexedElement("Property", propName, out xProperty))
                {
                    xProperty = this.DecompilerHelper.AddElementToRoot("Property");
                    xProperty.SetAttributeValue("Id", propName);
                }

                XElement xAccountSidSearch = new XElement(PanelSwWixExtension.Namespace + "AccountSidSearch");
                string systemName = row.FieldAsString(2);
                if (!string.IsNullOrEmpty(systemName))
                {
                    xAccountSidSearch.SetAttributeValue("SystemName", systemName);
                }
                string accountName = row.FieldAsString(3);
                if (!string.IsNullOrEmpty(accountName))
                {
                    xAccountSidSearch.SetAttributeValue("AccountName", accountName);
                }
                string condition = row.FieldAsString(4);
                if (!string.IsNullOrEmpty(condition))
                {
                    xAccountSidSearch.SetAttributeValue("Condition", condition);
                }

                xProperty.Add(xAccountSidSearch);
            }
        }

        private PanelSwWixCompiler.BackupAndRestore_deferred_Schedule? _backupAndRestoreSchedule;
        private void DecompileBackupAndRestore(Table table)
        {
            foreach (var row in table.Rows)
            {
                string component = row.FieldAsString(1);
                if (!this.DecompilerHelper.TryGetIndexedElement("Component", component, out XElement xComponent))
                {
                    this.Messaging.Write(WarningMessages.ExpectedForeignRow(row.SourceLineNumbers, table.Name, row.GetPrimaryKey(), "Component_", component, "Component"));
                    return;
                }

                XElement xBackupAndRestore = new XElement(PanelSwWixExtension.Namespace + "BackupAndRestore");
                xBackupAndRestore.SetAttributeValue("Path", row.FieldAsString(2));

                int flags = row.FieldAsInteger(3);
                if ((flags & (int)PanelSwWixCompiler.DeletePathFlags.IgnoreMissing) != 0)
                {
                    xBackupAndRestore.SetAttributeValue("IgnoreMissing", "yes");
                }
                if ((flags & (int)PanelSwWixCompiler.DeletePathFlags.IgnoreErrors) != 0)
                {
                    xBackupAndRestore.SetAttributeValue("IgnoreErrors", "yes");
                }

                if (_backupAndRestoreSchedule == PanelSwWixCompiler.BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_After_DuplicateFiles)
                {
                    xBackupAndRestore.SetAttributeValue("RestoreScheduling", "afterDuplicateFiles");
                }
                else if (_backupAndRestoreSchedule == PanelSwWixCompiler.BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_Before_InstallFiles)
                {
                    xBackupAndRestore.SetAttributeValue("RestoreScheduling", "beforeInstallFiles");
                }
                else if (_backupAndRestoreSchedule == PanelSwWixCompiler.BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_After_RemoveExistingProducts)
                {
                    xBackupAndRestore.SetAttributeValue("RestoreScheduling", "afterRemoveExistingProducts");
                }

                xComponent.Add(xBackupAndRestore);
            }
        }
    }
}
