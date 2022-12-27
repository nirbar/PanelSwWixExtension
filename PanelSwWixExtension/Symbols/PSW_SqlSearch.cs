using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_SqlSearch : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_SqlSearch), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_SqlSearch));
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
                    new ColumnDefinition(nameof(Server), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Instance), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Port), ColumnType.String, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Encrypted), ColumnType.String, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Database), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Username), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Password), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Query), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                    new ColumnDefinition(nameof(Order), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue),
                    new ColumnDefinition(nameof(ErrorHandling), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 2),
                    new ColumnDefinition(nameof(ConnectionString), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                };
            }
        }

        public PSW_SqlSearch() : base(SymbolDefinition)
        { }

        public PSW_SqlSearch(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "sql")
        { }

        public string Property_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string Server
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string Instance
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }

        public string Port
        {
            get => Fields[3].AsString();
            set => Fields[3].Set(value);
        }

        public string Encrypted
        {
            get => Fields[4].AsString();
            set => Fields[4].Set(value);
        }

        public string Database
        {
            get => Fields[5].AsString();
            set => Fields[5].Set(value);
        }

        public string Username
        {
            get => Fields[6].AsString();
            set => Fields[6].Set(value);
        }

        public string Password
        {
            get => Fields[7].AsString();
            set => Fields[7].Set(value);
        }

        public string Query
        {
            get => Fields[8].AsString();
            set => Fields[8].Set(value);
        }

        public string Condition
        {
            get => Fields[9].AsString();
            set => Fields[9].Set(value);
        }

        public int Order
        {
            get => Fields[10].AsNumber();
            set => Fields[10].Set(value);
        }

        public int ErrorHandling
        {
            get => Fields[11].AsNumber();
            set => Fields[11].Set(value);
        }

        public string ConnectionString
        {
            get => Fields[12].AsString();
            set => Fields[12].Set(value);
        }
    }
}
