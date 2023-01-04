using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_RemoveRegistryValue : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_RemoveRegistryValue), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_RemoveRegistryValue));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Root), ColumnType.String, 0, false, false, ColumnCategory.Text, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Key), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Name), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Area), ColumnType.String, 0, false, false, ColumnCategory.Text, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Attributes), ColumnType.Number, 2, false, true, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(Condition), ColumnType.Localized, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_RemoveRegistryValue() : base(SymbolDefinition)
        { }

        public PSW_RemoveRegistryValue(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "rrv")
        { }

        public string Root
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Key
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Name
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string Area
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public int Attributes
        {
            get => Fields[4].AsNumber();
            set => this.Set(4, value);
        }

        public string Condition
        {
            get => Fields[5].AsString();
            set => this.Set(5, value);
        }
    }
}
