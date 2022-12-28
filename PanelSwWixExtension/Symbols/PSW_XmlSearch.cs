using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_XmlSearch : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_XmlSearch), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_XmlSearch));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Property_), ColumnType.String, 0, false, false, ColumnCategory.Text, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(FilePath), ColumnType.String, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Expression), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Language), ColumnType.String, 0, false, true, ColumnCategory.Text, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Namespaces), ColumnType.String, 0, false, true, ColumnCategory.Text, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Match), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_XmlSearch() : base(SymbolDefinition)
        { }

        public PSW_XmlSearch(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "xml")
        { }

        public string Property_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string FilePath
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Expression
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string Language
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public string Namespaces
        {
            get => Fields[4].AsString();
            set => this.Set(4, value);
        }

        public int Match
        {
            get => Fields[5].AsNumber();
            set => this.Set(5, value);
        }

        public string Condition
        {
            get => Fields[6].AsString();
            set => this.Set(6, value);
        }
    }
}
