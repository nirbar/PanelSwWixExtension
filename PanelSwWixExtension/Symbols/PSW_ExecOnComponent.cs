using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ExecOnComponent : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ExecOnComponent), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_ExecOnComponent));
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
                    new ColumnDefinition(nameof(Binary_), ColumnType.String, 72, false, true, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Binary", keyColumn: 1),
                    new ColumnDefinition(nameof(Command), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(WorkingDirectory), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Flags), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: ushort.MaxValue),
                    new ColumnDefinition(nameof(ErrorHandling), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 2),
                    new ColumnDefinition(nameof(Order), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue),
                    new ColumnDefinition(nameof(User_), ColumnType.String, 72, false, true, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "User", keyColumn: 1),
                };
            }
        }

        public PSW_ExecOnComponent() : base(SymbolDefinition)
        { }

        public PSW_ExecOnComponent(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "exc")
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string Binary_
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string Command
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }

        public string WorkingDirectory
        {
            get => Fields[3].AsString();
            set => Fields[3].Set(value);
        }

        public int Flags
        {
            get => Fields[4].AsNumber();
            set => Fields[4].Set(value);
        }

        public int ErrorHandling
        {
            get => Fields[5].AsNumber();
            set => Fields[5].Set(value);
        }

        public int Order
        {
            get => Fields[6].AsNumber();
            set => Fields[6].Set(value);
        }

        public string User_
        {
            get => Fields[7].AsString();
            set => Fields[7].Set(value);
        }
    }
}