using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;
using static System.Net.Mime.MediaTypeNames;

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
                    new ColumnDefinition(nameof(SqlScript_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "File", keyColumn: 1)
                    , new ColumnDefinition(nameof(Text), ColumnType.Localized, 0, true, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Replacement), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Order), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue, modularizeType: ColumnModularizeType.None)
                };
            }
        }

        public PSW_SqlScript_Replacements() : base(SymbolDefinition)
        { }

        public PSW_SqlScript_Replacements(SourceLineNumber lineNumber, Identifier sqlId) : base(SymbolDefinition, lineNumber, sqlId)
        {
            SqlScript_ = sqlId.Id;
        }

        public string SqlScript_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string Text
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string Replacement
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }

        public int Order
        {
            get => Fields[3].AsNumber();
            set => Fields[3].Set(value);
        }
    }
}
