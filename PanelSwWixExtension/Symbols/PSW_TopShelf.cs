using System;
using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_TopShelf : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_TopShelf), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_TopShelf));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(File_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "File", keyColumn: 1)
                    , new ColumnDefinition(nameof(ServiceName), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(DisplayName), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Description), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Instance), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Account), ColumnType.Number, 2, false, false, ColumnCategory.Integer, 0, 4)
                    , new ColumnDefinition(nameof(UserName), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Password), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(HowToStart), ColumnType.Number, 2, false, false, ColumnCategory.Integer, 0, 4)
                    , new ColumnDefinition(nameof(ErrorHandling), ColumnType.Number, 2, false, false, ColumnCategory.Integer, 0, 2)
                };
            }
        }

        public PSW_TopShelf() : base(SymbolDefinition)
        { }

        public PSW_TopShelf(SourceLineNumber lineNumber, string file) : base(SymbolDefinition, lineNumber, new Identifier(AccessModifier.Global, file))
        {
            File_ = file;
        }

        public string File_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string ServiceName
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string DisplayName
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }

        public string Description
        {
            get => Fields[3].AsString();
            set => Fields[3].Set(value);
        }

        public string Instance
        {
            get => Fields[4].AsString();
            set => Fields[4].Set(value);
        }

        public int Account
        {
            get => Fields[5].AsNumber();
            set => Fields[5].Set(value);
        }

        public string UserName
        {
            get => Fields[6].AsString();
            set => Fields[6].Set(value);
        }

        public string Password
        {
            get => Fields[7].AsString();
            set => Fields[7].Set(value);
        }

        public int HowToStart
        {
            get => Fields[8].AsNumber();
            set => Fields[8].Set(value);
        }

        public int ErrorHandling
        {
            get => Fields[9].AsNumber();
            set => Fields[9].Set(value);
        }
    }
}
