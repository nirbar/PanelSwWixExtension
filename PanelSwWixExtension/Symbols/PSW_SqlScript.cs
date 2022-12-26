using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_SqlScript : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_SqlScript), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_SqlScript));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Component_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1)
                    , new ColumnDefinition(nameof(Binary_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Binary", keyColumn: 1)
                    , new ColumnDefinition(nameof(Server), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Instance), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Port), ColumnType.String, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Encrypted), ColumnType.String, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Database), ColumnType.String, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Username), ColumnType.String, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Password), ColumnType.String, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(On), ColumnType.Number, 2, false, false, ColumnCategory.Integer, 0, 127)
                    , new ColumnDefinition(nameof(ErrorHandling), ColumnType.Number, 2, false, false, ColumnCategory.Integer, 0, 2)
                    , new ColumnDefinition(nameof(Order), ColumnType.Number, 4, false, false, ColumnCategory.Integer, 0, int.MaxValue)
                    , new ColumnDefinition(nameof(ConnectionString), ColumnType.String, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Driver), ColumnType.String, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                };
            }
        }

        public PSW_SqlScript() : base(SymbolDefinition)
        { }

        public PSW_SqlScript(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "sql")
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string Binary_
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string Server
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }

        public string Instance
        {
            get => Fields[3].AsString();
            set => Fields[3].Set(value);
        }

        public string Port
        {
            get => Fields[4].AsString();
            set => Fields[4].Set(value);
        }

        public string Encrypted
        {
            get => Fields[5].AsString();
            set => Fields[5].Set(value);
        }

        public string Database
        {
            get => Fields[6].AsString();
            set => Fields[6].Set(value);
        }

        public string Username
        {
            get => Fields[7].AsString();
            set => Fields[7].Set(value);
        }

        public string Password
        {
            get => Fields[8].AsString();
            set => Fields[8].Set(value);
        }

        public int On
        {
            get => Fields[9].AsNumber();
            set => Fields[9].Set(value);
        }

        public int ErrorHandling
        {
            get => Fields[10].AsNumber();
            set => Fields[10].Set(value);
        }

        public int Order
        {
            get => Fields[11].AsNumber();
            set => Fields[11].Set(value);
        }

        public string ConnectionString
        {
            get => Fields[12].AsString();
            set => Fields[12].Set(value);
        }

        public string Driver
        {
            get => Fields[13].AsString();
            set => Fields[13].Set(value);
        }
    }
}
