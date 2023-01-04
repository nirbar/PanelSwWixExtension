using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_Dism : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_Dism), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_Dism));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Component_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1),
                    new ColumnDefinition(nameof(EnableFeatures), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(ExcludeFeatures), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(PackagePath), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Cost), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue),
                    new ColumnDefinition(nameof(ErrorHandling), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 2),
                };
            }
        }

        public PSW_Dism() : base(SymbolDefinition)
        { }

        public PSW_Dism(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "dsm")
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string EnableFeatures
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string ExcludeFeatures
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string PackagePath
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public int Cost
        {
            get => Fields[4].AsNumber();
            set => this.Set(4, value);
        }

        public int ErrorHandling
        {
            get => Fields[5].AsNumber();
            set => this.Set(5, value);
        }
    }
}
