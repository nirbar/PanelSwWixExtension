using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_JsonJPath : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_JsonJPath), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_JsonJPath));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Component_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1),
                    new ColumnDefinition(nameof(FilePath), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(File_), ColumnType.String, 72, false, true, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "File", keyColumn: 1),
                    new ColumnDefinition(nameof(JPath), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Value), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Formatting), ColumnType.Number, 1, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(ErrorHandling), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 2),
                };
            }
        }

        public PSW_JsonJPath() : base(SymbolDefinition)
        { }

        public PSW_JsonJPath(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "jpt")
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string FilePath
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string File_
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string JPath
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public string Value
        {
            get => Fields[4].AsString();
            set => this.Set(4, value);
        }

        public int Formatting
        {
            get => Fields[5].AsNumber();
            set => this.Set(5, value);
        }

        public int ErrorHandling
        {
            get => Fields[6].AsNumber();
            set => this.Set(6, value);
        }
    }
}
