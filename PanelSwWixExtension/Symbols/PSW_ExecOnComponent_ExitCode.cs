using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ExecOnComponent_ExitCode : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ExecOnComponent_ExitCode), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_ExecOnComponent_ExitCode));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(ExecOnId_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "PSW_ExecOnComponent", keyColumn: 1),
                    new ColumnDefinition(nameof(From), ColumnType.Number, 4, true, false, ColumnCategory.Integer, minValue: 0, maxValue: ushort.MaxValue),
                    new ColumnDefinition(nameof(To), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: ushort.MaxValue),
                };
            }
        }

        public PSW_ExecOnComponent_ExitCode() : base(SymbolDefinition)
        { }

        public PSW_ExecOnComponent_ExitCode(SourceLineNumber lineNumber, Identifier execOnId) : base(SymbolDefinition, lineNumber, "eoc")
        {
            ExecOnId_ = execOnId.Id;
        }

        public string ExecOnId_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public ushort From
        {
            get => (ushort)Fields[1].AsNumber();
            set => this.Set(1, value);
        }

        public ushort To
        {
            get => (ushort)Fields[2].AsNumber();
            set => this.Set(2, value);
        }
    }
}
