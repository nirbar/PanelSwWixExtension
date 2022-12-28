using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ServiceConfig_Dependency : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ServiceConfig_Dependency), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_ServiceConfig_Dependency));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(ServiceConfig_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "PSW_ServiceConfig", keyColumn: 1),
                    new ColumnDefinition(nameof(Service), ColumnType.String, 0, true, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Group), ColumnType.String, 0, true, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                };
            }
        }

        public PSW_ServiceConfig_Dependency() : base(SymbolDefinition)
        { }

        public PSW_ServiceConfig_Dependency(SourceLineNumber lineNumber, Identifier svcId) : base(SymbolDefinition, lineNumber, svcId)
        {
            ServiceConfig_ = svcId.Id;
        }

        public string ServiceConfig_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Service
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Group
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }
    }
}
