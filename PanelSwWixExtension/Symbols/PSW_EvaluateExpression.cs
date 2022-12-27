using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_EvaluateExpression : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_EvaluateExpression), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_EvaluateExpression));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Property_), ColumnType.String, 0, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Expression), ColumnType.Localized, 72, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Order), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue),
                };
            }
        }

        public PSW_EvaluateExpression() : base(SymbolDefinition)
        { }

        public PSW_EvaluateExpression(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "evl")
        { }

        public string Property_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string Expression
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public int Order
        {
            get => Fields[2].AsNumber();
            set => Fields[2].Set(value);
        }
    }
}
