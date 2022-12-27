using PanelSw.Wix.Extensions.Symbols;
using System;
using System.Collections.Generic;
using System.IO;
using WixToolset.Data;
using WixToolset.Data.Symbols;
using WixToolset.Data.WindowsInstaller;
using WixToolset.Extensibility;

namespace PanelSw.Wix.Extensions
{
    public class PanelSwWiBackendBinder : BaseWindowsInstallerBackendBinderExtension
    {
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
                        new TableDefinition(nameof(PSW_ConcatFiles), PSW_ConcatFiles.SymbolDefinition, PSW_ConcatFiles.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_CustomUninstallKey), PSW_CustomUninstallKey.SymbolDefinition, PSW_CustomUninstallKey.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_DeletePath), PSW_DeletePath.SymbolDefinition, PSW_DeletePath.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_DiskSpace), PSW_DiskSpace.SymbolDefinition, PSW_DiskSpace.ColumnDefinitions, symbolIdIsPrimaryKey: false), // No Id for this table, just the Directory_ column
                        new TableDefinition(nameof(PSW_Dism), PSW_Dism.SymbolDefinition, PSW_Dism.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_EvaluateExpression), PSW_EvaluateExpression.SymbolDefinition, PSW_EvaluateExpression.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ExecOn_ConsoleOutput), PSW_ExecOn_ConsoleOutput.SymbolDefinition, PSW_ExecOn_ConsoleOutput.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ExecOnComponent), PSW_ExecOnComponent.SymbolDefinition, PSW_ExecOnComponent.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ExecOnComponent_Environment), PSW_ExecOnComponent_Environment.SymbolDefinition, PSW_ExecOnComponent_Environment.ColumnDefinitions, symbolIdIsPrimaryKey: false), // No Id for this table, just the File column
                        new TableDefinition(nameof(PSW_ExecOnComponent_ExitCode), PSW_ExecOnComponent_ExitCode.SymbolDefinition, PSW_ExecOnComponent_ExitCode.ColumnDefinitions, symbolIdIsPrimaryKey: false), // No Id for this table, just the File column
                        new TableDefinition(nameof(PSW_FileRegex), PSW_FileRegex.SymbolDefinition, PSW_FileRegex.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ForceVersion), PSW_ForceVersion.SymbolDefinition, PSW_ForceVersion.ColumnDefinitions, symbolIdIsPrimaryKey: false), // No Id for this table, just the File column
                        new TableDefinition(nameof(PSW_InstallUtil), PSW_InstallUtil.SymbolDefinition, PSW_InstallUtil.ColumnDefinitions, symbolIdIsPrimaryKey: false), // No Id for this table, just the File_ column
                        new TableDefinition(nameof(PSW_InstallUtil_Arg), PSW_InstallUtil_Arg.SymbolDefinition, PSW_InstallUtil_Arg.ColumnDefinitions, symbolIdIsPrimaryKey: false), // No Id for this table, just the File_ column
                        new TableDefinition(nameof(PSW_JsonJPath), PSW_JsonJPath.SymbolDefinition, PSW_JsonJPath.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_JsonJpathSearch), PSW_JsonJpathSearch.SymbolDefinition, PSW_JsonJpathSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_Md5Hash), PSW_Md5Hash.SymbolDefinition, PSW_Md5Hash.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_MsiSqlQuery), PSW_MsiSqlQuery.SymbolDefinition, PSW_MsiSqlQuery.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_PathSearch), PSW_PathSearch.SymbolDefinition, PSW_PathSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_Payload), PSW_Payload.SymbolDefinition, PSW_Payload.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ReadIniValues), PSW_ReadIniValues.SymbolDefinition, PSW_ReadIniValues.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_RegularExpression), PSW_RegularExpression.SymbolDefinition, PSW_RegularExpression.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_RemoveRegistryValue), PSW_RemoveRegistryValue.SymbolDefinition, PSW_RemoveRegistryValue.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_RestartLocalResources), PSW_RestartLocalResources.SymbolDefinition, PSW_RestartLocalResources.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_SelfSignCertificate), PSW_SelfSignCertificate.SymbolDefinition, PSW_SelfSignCertificate.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ServiceConfig), PSW_ServiceConfig.SymbolDefinition, PSW_ServiceConfig.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ServiceConfig_Dependency), PSW_ServiceConfig_Dependency.SymbolDefinition, PSW_ServiceConfig_Dependency.ColumnDefinitions, symbolIdIsPrimaryKey: false), // No Id for this table, just the SqlScript_ column
                        new TableDefinition(nameof(PSW_SetPropertyFromPipe), PSW_SetPropertyFromPipe.SymbolDefinition, PSW_SetPropertyFromPipe.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ShellExecute), PSW_ShellExecute.SymbolDefinition, PSW_ShellExecute.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_SqlScript), PSW_SqlScript.SymbolDefinition, PSW_SqlScript.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_SqlScript_Replacements), PSW_SqlScript_Replacements.SymbolDefinition, PSW_SqlScript_Replacements.ColumnDefinitions, symbolIdIsPrimaryKey: false), // No Id for this table, just the SqlScript_ column
                        new TableDefinition(nameof(PSW_SqlSearch), PSW_SqlSearch.SymbolDefinition, PSW_SqlSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_TaskScheduler), PSW_TaskScheduler.SymbolDefinition, PSW_TaskScheduler.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_Telemetry), PSW_Telemetry.SymbolDefinition, PSW_Telemetry.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_ToLowerCase), PSW_ToLowerCase.SymbolDefinition, PSW_ToLowerCase.ColumnDefinitions, symbolIdIsPrimaryKey: false), // No Id for this table, just the Property_ column
                        new TableDefinition(nameof(PSW_TopShelf), PSW_TopShelf.SymbolDefinition, PSW_TopShelf.ColumnDefinitions, symbolIdIsPrimaryKey: false), // No Id for this table, just the File_ column
                        new TableDefinition(nameof(PSW_Unzip), PSW_Unzip.SymbolDefinition, PSW_Unzip.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_VersionCompare), PSW_VersionCompare.SymbolDefinition, PSW_VersionCompare.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_WebsiteConfig), PSW_WebsiteConfig.SymbolDefinition, PSW_WebsiteConfig.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_WmiSearch), PSW_WmiSearch.SymbolDefinition, PSW_WmiSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_XmlSearch), PSW_XmlSearch.SymbolDefinition, PSW_XmlSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_XslTransform), PSW_XslTransform.SymbolDefinition, PSW_XslTransform.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                        new TableDefinition(nameof(PSW_XslTransform_Replacements), PSW_XslTransform_Replacements.SymbolDefinition, PSW_XslTransform_Replacements.ColumnDefinitions, symbolIdIsPrimaryKey: false), // No Id for this table, just the XslTransform_ column
                        new TableDefinition(nameof(PSW_ZipFile), PSW_ZipFile.SymbolDefinition, PSW_ZipFile.ColumnDefinitions, symbolIdIsPrimaryKey: true),
                    };
                }

                return _tableDefinition;
            }
        }

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
            SplitFiles(section);
        }

        private void SplitFiles(IntermediateSection section)
        {
            // This section is empty, need to iterate the other sections
            List<PSW_ConcatFiles> concatFiles = new List<PSW_ConcatFiles>();
            foreach (IntermediateSection intermediate in base.Context.IntermediateRepresentation.Sections)
            {
                foreach (IntermediateSymbol symbol in intermediate.Symbols)
                {
                    if (symbol is PSW_ConcatFiles concatSymbol)
                    {
                        concatFiles.Add(concatSymbol);
                    }
                }
            }

            concatFiles.Sort(new ConcatFilesComparer());

            string tmpPath = Path.GetTempPath();
            Table wixFileTable = output.Tables["WixFile"];
            FileSymbol rootWixFile = null;
            int splitSize = Int32.MaxValue;
            FileStream rootFileStream = null;
            try
            {
                foreach (PSW_ConcatFiles concatSymbol in concatFiles)
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
    }
}
