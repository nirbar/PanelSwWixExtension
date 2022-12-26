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
                        new TableDefinition(nameof(PSW_ConcatFiles), PSW_ConcatFiles.SymbolDefinition, PSW_ConcatFiles.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_Payload), PSW_Payload.SymbolDefinition, PSW_Payload.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_ReadIniValues), PSW_ReadIniValues.SymbolDefinition, PSW_ReadIniValues.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                        , new TableDefinition(nameof(PSW_WebsiteConfig), PSW_WebsiteConfig.SymbolDefinition, PSW_WebsiteConfig.ColumnDefinitions, symbolIdIsPrimaryKey: true)
                    };
                }

                return _tableDefinition;
            }
        }
    }
}
