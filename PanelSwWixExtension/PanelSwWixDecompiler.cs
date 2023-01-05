using PanelSw.Wix.Extensions.Symbols;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml.Linq;
using System.Xml.XPath;
using WixToolset.Data;
using WixToolset.Data.Symbols;
using WixToolset.Data.WindowsInstaller;
using WixToolset.Extensibility;
using WixToolset.Extensibility.Data;

namespace PanelSw.Wix.Extensions
{
    public class PanelSwWixDecompiler : BaseWindowsInstallerDecompilerExtension
    {
        public override IReadOnlyCollection<TableDefinition> TableDefinitions => PanelSwWixExtension.TableDefinitions;

        public override bool TryDecompileTable(Table table)
        {
            switch (table.Name)
            {
                case nameof(PSW_AccountSidSearch):
                    DecompileAccountSidSearch(table);
                    break;
                case nameof(PSW_BackupAndRestore):
                    DecompileBackupAndRestore(table);
                    break;
                case nameof(PSW_CertificateHashSearch):
                    DecompileCertificateHashSearch(table);
                    break;
                case nameof(PSW_ConcatFiles):
                    DecompileConcatFiles(table);
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

                            propValue = propValue.Replace("TerminateSuccessfully_Deferred", "");
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
                    else if (propName.Equals("SecureCustomProperties"))
                    {
                        string propValue = propRow.FieldAsString(1);
                        if (!string.IsNullOrEmpty(propValue))
                        {
                            if (propValue.Contains("DOMAIN_ADMINISTRATORS"))
                            {
                                XElement caRef = DecompilerHelper.AddElementToRoot("CustomActionRef");
                                caRef.SetAttributeValue("Id", "AccountNames");
                            }
                        }
                    }
                }
            }

            // PanelSw actions
            if (tables.TryGetTable("InstallUISequence", out Table installUISequence))
            {
                foreach (string a in new string[] { "TerminateSuccessfully_Immediate", "Rollback_Immediate", "PromptCancel_Immediate" })
                {
                    Row actionRow = installUISequence.Rows.FirstOrDefault(r => a.Equals(r[0]?.ToString()));
                    if (actionRow != null)
                    {
                        XElement caRef = DecompilerHelper.AddElementToRoot("CustomActionRef");
                        caRef.SetAttributeValue("Id", a);

                        XElement uiSeq = DecompilerHelper.AddElementToRoot(SequenceTable.InstallUISequence.ToString());
                        XElement caAction = DecompilerHelper.CreateElement("Custom");
                        caAction.SetAttributeValue("Action", a);
                        caAction.SetAttributeValue("Condition", actionRow.Fields[1]?.ToString());
                        caAction.SetAttributeValue("Sequence", actionRow.Fields[2]?.ToString());
                        uiSeq.Add(caAction);
                    }
                }
            }
            if (tables.TryGetTable("InstallExecuteSequence", out Table installExeSequence))
            {
                foreach (string a in new string[] { "TerminateSuccessfully_Immediate", "TerminateSuccessfully_Deferred", "Rollback_Immediate", "Rollback_Deferred", "PromptCancel_Immediate", "PromptCancel_Deferred" })
                {
                    Row actionRow = installUISequence.Rows.FirstOrDefault(r => a.Equals(r[0]?.ToString()));
                    if (actionRow != null)
                    {
                        XElement caRef = DecompilerHelper.AddElementToRoot("CustomActionRef");
                        caRef.SetAttributeValue("Id", a);

                        XElement uiSeq = DecompilerHelper.AddElementToRoot(SequenceTable.InstallUISequence.ToString());
                        XElement caAction = DecompilerHelper.CreateElement("Custom");
                        caAction.SetAttributeValue("Action", a);
                        caAction.SetAttributeValue("Condition", actionRow.Fields[1]?.ToString());
                        caAction.SetAttributeValue("Sequence", actionRow.Fields[2]?.ToString());
                        uiSeq.Add(caAction);
                    }
                }
            }
        }

        public override void PostDecompileTables(TableIndexedCollection tables)
        {
            MergeSplitFiles();
        }

        private void MergeSplitFiles()
        {
            _concatFiles.Sort(new ConcatFilesComparer());
            foreach (PSW_ConcatFiles concatFile in _concatFiles)
            {
                XElement compFile = DecompilerHelper.RootElement.XPathSelectElement($"//*[local-name() = 'File' and @Id='{concatFile.MyFile_}']");
                if (!string.IsNullOrEmpty(Context.ExtractFolder))
                {
                    XElement rootFile = DecompilerHelper.RootElement.XPathSelectElement($"//*[local-name() = 'File' and @Id='{concatFile.RootFile_}']");
                    string rootSource = rootFile.Attribute("Source")?.Value.Substring("SourceDir\\".Length);
                    rootSource = Path.Combine(Context.ExtractFolder, rootSource);

                    string compSource = compFile.Attribute("Source")?.Value.Substring("SourceDir\\".Length);
                    compSource = Path.Combine(Context.ExtractFolder, compSource);

                    // Merge to root
                    if (File.Exists(compSource) && File.Exists(rootSource))
                    {
                        using (FileStream wf = File.OpenWrite(rootSource))
                        {
                            wf.Seek(0, SeekOrigin.End);
                            using (FileStream rf = File.OpenRead(compSource))
                            {
                                rf.CopyTo(wf);
                            }
                        }
                        File.Delete(compSource);
                    }

                }
                compFile.Remove();
            }
        }

        private List<PSW_ConcatFiles> _concatFiles = new List<PSW_ConcatFiles>();
        private void DecompileConcatFiles(Table table)
        {
            foreach (var row in table.Rows)
            {
                PSW_ConcatFiles symbol = new PSW_ConcatFiles();
                symbol.LoadFromRow(row, out string junk);
                _concatFiles.Add(symbol);

                // Already added split file
                if (symbol.Order > 1)
                {
                    continue;
                }
                if (!DecompilerHelper.TryGetIndexedElement("File", symbol.RootFile_, out XElement xRootFile))
                {
                    Messaging.Write(WarningMessages.ExpectedForeignRow(row.SourceLineNumbers, table.Name, row.GetPrimaryKey(), "RootFile_", symbol.RootFile_, "File"));
                    return;
                }

                XElement xSplitFile = new XElement(PanelSwWixExtension.Namespace + "SplitFile");
                xSplitFile.SetAttributeValue(nameof(symbol.Size), symbol.Size);

                xRootFile.Add(xSplitFile);
            }
        }

        private void DecompileCertificateHashSearch(Table table)
        {
            foreach (var row in table.Rows)
            {
                PSW_CertificateHashSearch symbol = new PSW_CertificateHashSearch();
                symbol.LoadFromRow(row, out string propName);

                XElement xProperty = null;
                if (!DecompilerHelper.TryGetIndexedElement("Property", propName, out xProperty))
                {
                    xProperty = DecompilerHelper.AddElementToRoot("Property");
                    xProperty.SetAttributeValue("Id", propName);
                }

                XElement xCertificateHashSearch = new XElement(PanelSwWixExtension.Namespace + "CertificateHashSearch");
                if (!string.IsNullOrEmpty(symbol.CertName))
                {
                    xCertificateHashSearch.SetAttributeValue(nameof(symbol.CertName), symbol.CertName);
                }
                if (!string.IsNullOrEmpty(symbol.FriendlyName))
                {
                    xCertificateHashSearch.SetAttributeValue(nameof(symbol.FriendlyName), symbol.FriendlyName);
                }
                if (!string.IsNullOrEmpty(symbol.Issuer))
                {
                    xCertificateHashSearch.SetAttributeValue(nameof(symbol.Issuer), symbol.Issuer);
                }
                if (!string.IsNullOrEmpty(symbol.SerialNumber))
                {
                    xCertificateHashSearch.SetAttributeValue(nameof(symbol.SerialNumber), symbol.SerialNumber);
                }

                xProperty.Add(xCertificateHashSearch);
            }
        }

        private void DecompileAccountSidSearch(Table table)
        {
            foreach (var row in table.Rows)
            {
                PSW_AccountSidSearch symbol = new PSW_AccountSidSearch();
                symbol.LoadFromRow(row, out string propName);

                XElement xProperty = null;
                if (!DecompilerHelper.TryGetIndexedElement("Property", propName, out xProperty))
                {
                    xProperty = DecompilerHelper.AddElementToRoot("Property");
                    xProperty.SetAttributeValue("Id", propName);
                }

                XElement xAccountSidSearch = new XElement(PanelSwWixExtension.Namespace + "AccountSidSearch");
                if (!string.IsNullOrEmpty(symbol.SystemName))
                {
                    xAccountSidSearch.SetAttributeValue(nameof(symbol.SystemName), symbol.SystemName);
                }
                if (!string.IsNullOrEmpty(symbol.AccountName))
                {
                    xAccountSidSearch.SetAttributeValue(nameof(symbol.AccountName), symbol.AccountName);
                }
                if (!string.IsNullOrEmpty(symbol.Condition))
                {
                    xAccountSidSearch.SetAttributeValue(nameof(symbol.Condition), symbol.Condition);
                }

                xProperty.Add(xAccountSidSearch);
            }
        }

        private PanelSwWixCompiler.BackupAndRestore_deferred_Schedule? _backupAndRestoreSchedule;
        private void DecompileBackupAndRestore(Table table)
        {
            foreach (var row in table.Rows)
            {
                PSW_BackupAndRestore symbol = new PSW_BackupAndRestore();
                symbol.LoadFromRow(row, out string junk);

                if (!DecompilerHelper.TryGetIndexedElement("Component", symbol.Component_, out XElement xComponent))
                {
                    Messaging.Write(WarningMessages.ExpectedForeignRow(row.SourceLineNumbers, table.Name, row.GetPrimaryKey(), "Component_", symbol.Component_, "Component"));
                    return;
                }

                XElement xBackupAndRestore = new XElement(PanelSwWixExtension.Namespace + "BackupAndRestore");
                xBackupAndRestore.SetAttributeValue("Path", symbol.Path);

                if ((symbol.Flags & (int)PanelSwWixCompiler.DeletePathFlags.IgnoreMissing) != 0)
                {
                    xBackupAndRestore.SetAttributeValue("IgnoreMissing", "yes");
                }
                if ((symbol.Flags & (int)PanelSwWixCompiler.DeletePathFlags.IgnoreErrors) != 0)
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
