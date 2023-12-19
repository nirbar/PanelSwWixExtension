using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_DuplicateFolder : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_DuplicateFolder), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_DuplicateFolder));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(SourceDir_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Directory", keyColumn: 1),
                    new ColumnDefinition(nameof(DestinationDir_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Directory", keyColumn: 1),
                };
            }
        }

        public PSW_DuplicateFolder() : base(SymbolDefinition)
        { }

        public PSW_DuplicateFolder(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "dpf")
        { }

        public string SourceDir_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string DestinationDir_
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }
    }
}
