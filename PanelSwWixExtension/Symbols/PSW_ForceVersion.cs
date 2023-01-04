using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ForceVersion : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ForceVersion), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_ForceVersion));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(File), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "File", keyColumn: 1),
                    new ColumnDefinition(nameof(Version), ColumnType.Localized, 72, false, false, ColumnCategory.Version),
                };
            }
        }

        public PSW_ForceVersion() : base(SymbolDefinition)
        { }

        public PSW_ForceVersion(SourceLineNumber lineNumber, string file) : base(SymbolDefinition, lineNumber, new Identifier(AccessModifier.Global, file))
        {
            File = file;
        }

        public string File
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Version
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }
    }
}
