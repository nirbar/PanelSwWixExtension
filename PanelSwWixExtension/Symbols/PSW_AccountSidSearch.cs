using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_AccountSidSearch : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_AccountSidSearch), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_AccountSidSearch));
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
                    new ColumnDefinition(nameof(SystemName), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(AccountName), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_AccountSidSearch() : base(SymbolDefinition)
        { }

        public PSW_AccountSidSearch(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "sid")
        { }

        public string Property_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string SystemName
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string AccountName
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }

        public string Condition
        {
            get => Fields[3].AsString();
            set => Fields[3].Set(value);
        }
    }
}
