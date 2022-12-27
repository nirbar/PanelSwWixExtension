using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ServiceConfig : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ServiceConfig), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_ServiceConfig));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Component_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1),
                    new ColumnDefinition(nameof(ServiceName), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(CommandLine), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Account), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Password), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Start), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: -1, maxValue: 4),
                    new ColumnDefinition(nameof(DelayStart), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: -1, maxValue: 1),
                    new ColumnDefinition(nameof(LoadOrderGroup), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(ErrorHandling), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 2),
                };
            }
        }

        public PSW_ServiceConfig() : base(SymbolDefinition)
        { }

        public PSW_ServiceConfig(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "svc")
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string ServiceName
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string CommandLine
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }

        public string Account
        {
            get => Fields[3].AsString();
            set => Fields[3].Set(value);
        }

        public string Password
        {
            get => Fields[4].AsString();
            set => Fields[4].Set(value);
        }

        public short Start
        {
            get => (short)Fields[5].AsNumber();
            set => Fields[5].Set(value);
        }

        public short DelayStart
        {
            get => (short)Fields[6].AsNumber();
            set => Fields[6].Set(value);
        }

        public string LoadOrderGroup
        {
            get => Fields[7].AsString();
            set => Fields[7].Set(value);
        }

        public short ErrorHandling
        {
            get => (short)Fields[8].AsNumber();
            set => Fields[8].Set(value);
        }
    }
}
