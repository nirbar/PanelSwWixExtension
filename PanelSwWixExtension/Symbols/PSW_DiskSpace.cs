using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_DiskSpace : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_DiskSpace), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_DiskSpace));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Directory_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Directory", keyColumn: 1)
                };
            }
        }

        public PSW_DiskSpace() : base(SymbolDefinition)
        { }

        public PSW_DiskSpace(SourceLineNumber lineNumber, string directory) : base(SymbolDefinition, lineNumber, new Identifier(AccessModifier.Global, directory))
        {
            Directory_ = directory;
        }

        public string Directory_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }
    }
}
