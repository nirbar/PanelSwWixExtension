using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_WmiSearch : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_WmiSearch), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_WmiSearch));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Property_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                    new ColumnDefinition(nameof(Namespace), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Query), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(ResultProperty), ColumnType.String, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Order), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue),
                };
            }
        }

        public PSW_WmiSearch() : base(SymbolDefinition)
        { }

        public PSW_WmiSearch(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "wmi")
        { }

        public string Property_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Condition
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Namespace
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string Query
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public string ResultProperty
        {
            get => Fields[4].AsString();
            set => this.Set(4, value);
        }

        public int Order
        {
            get => Fields[5].AsNumber();
            set => this.Set(5, value);
        }
    }
}
