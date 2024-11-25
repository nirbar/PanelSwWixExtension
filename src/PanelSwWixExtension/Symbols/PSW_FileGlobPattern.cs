using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_FileGlobPattern : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_FileGlobPattern), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_FileGlobPattern));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.None),
                    new ColumnDefinition(nameof(FileGlob_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.None, keyTable: "PSW_FileGlob", keyColumn: 1),
                    new ColumnDefinition(nameof(Include), ColumnType.String, 72, false, true, ColumnCategory.Text, modularizeType: ColumnModularizeType.None),
                    new ColumnDefinition(nameof(Exclude), ColumnType.String, 72, false, true, ColumnCategory.Text, modularizeType: ColumnModularizeType.None),
                };
            }
        }

        public PSW_FileGlobPattern() : base(SymbolDefinition)
        { }

        public PSW_FileGlobPattern(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "glp")
        { }

        public string FileGlob_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Include
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Exclude
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }
    }
}
