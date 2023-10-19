using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ZipFile : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ZipFile), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_ZipFile));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(ZipFile), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(CompressFolder), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(FilePattern), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Recursive), ColumnType.Number, 1, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 1),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                    new ColumnDefinition(nameof(ErrorHandling), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 2),
                };
            }
        }

        public PSW_ZipFile() : base(SymbolDefinition)
        { }

        public PSW_ZipFile(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "zip")
        { }

        public string ZipFile
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string CompressFolder
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string FilePattern
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public int Recursive
        {
            get => Fields[3].AsNumber();
            set => this.Set(3, value);
        }

        public string Condition
        {
            get => Fields[4].AsString();
            set => this.Set(4, value);
        }

        public int ErrorHandling
        {
            get => Fields[5].AsNumber();
            set => this.Set(5, value);
        }
    }
}
