using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_MsiSqlQuery : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_MsiSqlQuery), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_MsiSqlQuery));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Property_), ColumnType.String, 0, false, true, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Query), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_MsiSqlQuery() : base(SymbolDefinition)
        { }

        public PSW_MsiSqlQuery(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "msq")
        { }

        public string Property_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Query
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Condition
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }
    }
}
