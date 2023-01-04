using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_SqlScript_Replacements : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_SqlScript_Replacements), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_SqlScript_Replacements));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(SqlScript_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "PSW_SqlScript", keyColumn: 1),
                    new ColumnDefinition(nameof(Text), ColumnType.String, 0, true, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Replacement), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Order), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue),
                };
            }
        }

        public PSW_SqlScript_Replacements() : base(SymbolDefinition)
        { }

        public PSW_SqlScript_Replacements(SourceLineNumber lineNumber, Identifier sqlId) : base(SymbolDefinition, lineNumber, "sqr")
        {
            SqlScript_ = sqlId.Id;
        }

        public string SqlScript_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Text
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Replacement
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public int Order
        {
            get => Fields[3].AsNumber();
            set => this.Set(3, value);
        }
    }
}
