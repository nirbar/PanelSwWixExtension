using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_RegularExpression : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_RegularExpression), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_RegularExpression));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(FilePath), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Input), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Expression), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Replacement), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(DstProperty_), ColumnType.String, 0, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Flags), ColumnType.Number, 2, false, true, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                    new ColumnDefinition(nameof(Order), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue),
                };
            }
        }

        public PSW_RegularExpression() : base(SymbolDefinition)
        { }

        public PSW_RegularExpression(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "rgx")
        { }

        public string FilePath
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string Input
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string Expression
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }

        public string Replacement
        {
            get => Fields[3].AsString();
            set => Fields[3].Set(value);
        }

        public string DstProperty_
        {
            get => Fields[4].AsString();
            set => Fields[4].Set(value);
        }

        public int Flags
        {
            get => Fields[5].AsNumber();
            set => Fields[5].Set(value);
        }

        public string Condition
        {
            get => Fields[6].AsString();
            set => Fields[6].Set(value);
        }

        public int Order
        {
            get => Fields[7].AsNumber();
            set => Fields[7].Set(value);
        }
    }
}
