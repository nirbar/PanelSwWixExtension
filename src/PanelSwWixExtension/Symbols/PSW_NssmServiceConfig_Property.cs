using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_NssmServiceConfig_Property : BaseSymbol
    {
        public enum NssmRegDataType
        {
            REG_NONE = 0,
            REG_SZ = 1,
            REG_EXPAND_SZ = 2,
            REG_DWORD = 4,
            REG_MULTI_SZ = 7,
        }

        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_NssmServiceConfig_Property), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_NssmServiceConfig_Property));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(PSW_NssmServiceConfig_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "PSW_NssmServiceConfig", keyColumn: 1),
                    new ColumnDefinition(nameof(Name), ColumnType.String, 0, false, false, ColumnCategory.Text, modularizeType: ColumnModularizeType.None),
                    new ColumnDefinition(nameof(Value), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(DataType), ColumnType.Number, 0, false, false, ColumnCategory.Integer, modularizeType: ColumnModularizeType.None),
                };
            }
        }

        public PSW_NssmServiceConfig_Property() : base(SymbolDefinition)
        { }

        public PSW_NssmServiceConfig_Property(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "svc")
        { }

        public string PSW_NssmServiceConfig_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Name
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Value
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public NssmRegDataType DataType
        {
            get => (NssmRegDataType)Fields[3].AsNumber();
            set => this.Set(3, (int)value);
        }
    }
}
