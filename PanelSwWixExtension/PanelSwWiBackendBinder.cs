using PanelSw.Wix.Extensions.Symbols;
using System.Collections.Generic;
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
                        new TableDefinition(nameof(PSW_BackupAndRestore), PSW_BackupAndRestore.SymbolDefinition, PSW_BackupAndRestore.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_ConcatFiles), PSW_ConcatFiles.SymbolDefinition, PSW_ConcatFiles.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_DiskSpace), PSW_DiskSpace.SymbolDefinition, PSW_DiskSpace.ColumnDefinitions, symbolIdIsPrimaryKey: false) // No Id for this table, just the Directory_ column
                        , new TableDefinition(nameof(PSW_ForceVersion), PSW_ForceVersion.SymbolDefinition, PSW_ForceVersion.ColumnDefinitions, symbolIdIsPrimaryKey: false) // No Id for this table, just the File column
                        , new TableDefinition(nameof(PSW_InstallUtil), PSW_InstallUtil.SymbolDefinition, PSW_InstallUtil.ColumnDefinitions, symbolIdIsPrimaryKey: false) // No Id for this table, just the File_ column
                        , new TableDefinition(nameof(PSW_InstallUtil_Arg), PSW_InstallUtil_Arg.SymbolDefinition, PSW_InstallUtil_Arg.ColumnDefinitions, symbolIdIsPrimaryKey: false) // No Id for this table, just the File_ column
                        , new TableDefinition(nameof(PSW_JsonJPath), PSW_JsonJPath.SymbolDefinition, PSW_JsonJPath.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_JsonJpathSearch), PSW_JsonJpathSearch.SymbolDefinition, PSW_JsonJpathSearch.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_Md5Hash), PSW_Md5Hash.SymbolDefinition, PSW_Md5Hash.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_Payload), PSW_Payload.SymbolDefinition, PSW_Payload.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_ReadIniValues), PSW_ReadIniValues.SymbolDefinition, PSW_ReadIniValues.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_SelfSignCertificate), PSW_SelfSignCertificate.SymbolDefinition, PSW_SelfSignCertificate.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_SetPropertyFromPipe), PSW_SetPropertyFromPipe.SymbolDefinition, PSW_SetPropertyFromPipe.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_SqlScript), PSW_SqlScript.SymbolDefinition, PSW_SqlScript.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_SqlScript_Replacements), PSW_SqlScript_Replacements.SymbolDefinition, PSW_SqlScript_Replacements.ColumnDefinitions, symbolIdIsPrimaryKey: false) // No Id for this table, just the SqlScript_ column
                        , new TableDefinition(nameof(PSW_ToLowerCase), PSW_ToLowerCase.SymbolDefinition, PSW_ToLowerCase.ColumnDefinitions, symbolIdIsPrimaryKey: false) // No Id for this table, just the Property_ column
                        , new TableDefinition(nameof(PSW_TopShelf), PSW_TopShelf.SymbolDefinition, PSW_TopShelf.ColumnDefinitions, symbolIdIsPrimaryKey: false) // No Id for this table, just the File_ column
                        , new TableDefinition(nameof(PSW_WebsiteConfig), PSW_WebsiteConfig.SymbolDefinition, PSW_WebsiteConfig.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_XslTransform), PSW_XslTransform.SymbolDefinition, PSW_XslTransform.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_XslTransform_Replacements), PSW_XslTransform_Replacements.SymbolDefinition, PSW_XslTransform_Replacements.ColumnDefinitions, symbolIdIsPrimaryKey: false) // No Id for this table, just the XslTransform_ column
                    };
                }

                return _tableDefinition;
            }
        }
    }
}
