using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_Unzip : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_Unzip), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_Unzip));
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
                    new ColumnDefinition(nameof(TargetFolder), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Flags), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: ushort.MaxValue),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_Unzip() : base(SymbolDefinition)
        { }

        public PSW_Unzip(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "uzp")
        { }

        public string ZipFile
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string TargetFolder
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public int Flags
        {
            get => Fields[2].AsNumber();
            set => this.Set(2, value);
        }

        public string Condition
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }
    }
}
