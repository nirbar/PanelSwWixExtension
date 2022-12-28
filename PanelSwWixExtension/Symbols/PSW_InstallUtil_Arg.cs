using System;
using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_InstallUtil_Arg : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_InstallUtil_Arg), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_InstallUtil_Arg));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(File_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "File", keyColumn: 1),
                    new ColumnDefinition(nameof(Id), ColumnType.String, 0, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Value), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                };
            }
        }

        public PSW_InstallUtil_Arg() : base(SymbolDefinition)
        { }

        public PSW_InstallUtil_Arg(SourceLineNumber lineNumber, string file) : base(SymbolDefinition, lineNumber, "iua")
        {
            File_ = file;
        }

        public string File_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        // Field #1 is Id

        public string Value
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }
    }
}
