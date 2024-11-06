using System;
using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_IsWindowsVersionOrGreater : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_IsWindowsVersionOrGreater), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_IsWindowsVersionOrGreater));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Property_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(MinVersion), ColumnType.String, 0, false, false, ColumnCategory.Version, modularizeType: ColumnModularizeType.None),
                    new ColumnDefinition(nameof(MaxVersion), ColumnType.String, 0, false, true, ColumnCategory.Version, modularizeType: ColumnModularizeType.None),
                };
            }
        }

        public PSW_IsWindowsVersionOrGreater() : base(SymbolDefinition)
        { }

        public PSW_IsWindowsVersionOrGreater(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "wmv")
        { }

        public string Property_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public Version MinVersion
        {
            get => Version.TryParse(Fields[1].AsString(), out Version v) ? v : new Version(0, 0, 0, 0);
            set => this.Set(1, value.ToString());
        }

        public Version MaxVersion
        {
            get => Version.TryParse(Fields[2].AsString(), out Version v) ? v : new Version(0, 0, 0, 0);
            set => this.Set(2, value.ToString());
        }
    }
}
