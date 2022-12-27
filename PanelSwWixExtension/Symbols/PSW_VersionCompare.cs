using System;
using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_VersionCompare : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_VersionCompare), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_VersionCompare));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Property_), ColumnType.String, 0, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Version1), ColumnType.String, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Version2), ColumnType.String, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                };
            }
        }

        public PSW_VersionCompare() : base(SymbolDefinition)
        { }

        public PSW_VersionCompare(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "ver")
        { }

        public string Property_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string Version1
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string Version2
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }
    }
}
