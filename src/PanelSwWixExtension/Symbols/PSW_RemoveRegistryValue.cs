using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.Symbols;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_RemoveRegistryValue : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_RemoveRegistryValue), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_RemoveRegistryValue));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Component_), ColumnType.String, 72, false, true, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1),
                    new ColumnDefinition(nameof(Root), ColumnType.Number, 0, false, false, ColumnCategory.Integer, modularizeType: ColumnModularizeType.None),
                    new ColumnDefinition(nameof(Key), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Name), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(View), ColumnType.Number, 0, false, false, ColumnCategory.Integer, modularizeType: ColumnModularizeType.None),
                    new ColumnDefinition(nameof(Condition), ColumnType.Localized, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_RemoveRegistryValue() : base(SymbolDefinition)
        { }

        public PSW_RemoveRegistryValue(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "rrv")
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public RegistryRootType Root
        {
            get => (RegistryRootType)Fields[1].AsNumber();
            set => this.Set(1, (int)value);
        }

        public string Key
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string Name
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public YesNoDefaultType IsX64
        {
            get => (View == KEY_WOW64_32KEY) ? YesNoDefaultType.No
                : (View == KEY_WOW64_64KEY) ? YesNoDefaultType.Yes
                : YesNoDefaultType.Default;
            set
            {
                switch (value)
                {
                    case YesNoDefaultType.No:
                        View = KEY_WOW64_32KEY;
                        break;
                    case YesNoDefaultType.Yes:
                        View = KEY_WOW64_64KEY;
                        break;
                    case YesNoDefaultType.Default:
                        View = KEY_WOW64_UNKNOWN;
                        break;
                }
            }
        }

        public int View
        {
            get => Fields[4].AsNumber();
            set => this.Set(4, value);
        }

        public string Condition
        {
            get => Fields[5].AsString();
            set => this.Set(5, value);
        }

        private const int KEY_WOW64_UNKNOWN = -1;
        private const int KEY_WOW64_64KEY = 0x100;
        private const int KEY_WOW64_32KEY = 0x200;
    }
}
