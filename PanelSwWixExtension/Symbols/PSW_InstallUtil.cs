using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_InstallUtil : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_InstallUtil), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_InstallUtil));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(File_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "File", keyColumn: 1),
                    new ColumnDefinition(nameof(Bitness), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                };
            }
        }

        public PSW_InstallUtil() : base(SymbolDefinition)
        { }

        public PSW_InstallUtil(SourceLineNumber lineNumber, string file) : base(SymbolDefinition, lineNumber, new Identifier(AccessModifier.Global, file))
        {
            File_ = file;
        }

        public string File_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public ushort Bitness
        {
            get => (ushort)Fields[1].AsNumber();
            set => Fields[1].Set(value);
        }
    }
}
