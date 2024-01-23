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
                    new ColumnDefinition(nameof(File_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "File", keyColumn: 1),
                    new ColumnDefinition(nameof(ServiceName), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(DisplayName), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Description), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Instance), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Account), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 4),
                    new ColumnDefinition(nameof(UserName), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Password), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(HowToStart), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 4),
                    new ColumnDefinition(nameof(ErrorHandling), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 2),
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
            set => this.Set(0, value);
        }

        public string ServiceName
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string DisplayName
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string Description
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public string Instance
        {
            get => Fields[4].AsString();
            set => this.Set(4, value);
        }

        public int Account
        {
            get => Fields[5].AsNumber();
            set => this.Set(5, value);
        }

        public string UserName
        {
            get => Fields[6].AsString();
            set => this.Set(6, value);
        }

        public string Password
        {
            get => Fields[7].AsString();
            set => this.Set(7, value);
        }

        public int HowToStart
        {
            get => Fields[8].AsNumber();
            set => this.Set(8, value);
        }

        public int ErrorHandling
        {
            get => Fields[9].AsNumber();
            set => this.Set(9, value);
        }
    }
}
