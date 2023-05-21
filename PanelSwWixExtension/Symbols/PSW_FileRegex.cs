using System;
using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_FileRegex : BaseSymbol, IComparable<PSW_FileRegex>
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_FileRegex), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_FileRegex));
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
                    new ColumnDefinition(nameof(File_), ColumnType.String, 72, false, true, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "File", keyColumn: 1),
                    new ColumnDefinition(nameof(FilePath), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Regex), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Replacement), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(IgnoreCase), ColumnType.Number, 2, false, true, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(Encoding), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                    new ColumnDefinition(nameof(Order), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue),
                };
            }
        }

        public PSW_FileRegex() : base(SymbolDefinition)
        { }

        public PSW_FileRegex(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "frx")
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string File_
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string FilePath
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string Regex
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public string Replacement
        {
            get => Fields[4].AsString();
            set => this.Set(4, value);
        }

        public int IgnoreCase
        {
            get => Fields[5].AsNumber();
            set => this.Set(5, value);
        }

        public int Encoding
        {
            get => Fields[6].AsNumber();
            set => this.Set(6, value);
        }

        public string Condition
        {
            get => Fields[7].AsString();
            set => this.Set(7, value);
        }

        public int Order
        {
            get => Fields[8].AsNumber();
            set => this.Set(8, value);
        }

        int IComparable<PSW_FileRegex>.CompareTo(PSW_FileRegex other)
        {
            if (string.IsNullOrEmpty(File_))
            {
                return string.IsNullOrEmpty(other.File_) ? Order.CompareTo(other.Order) : 1;
            }
            if (string.IsNullOrEmpty(other.File_))
            {
                return -1;
            }
            int fileComp = File_.CompareTo(other.File_);
            return (fileComp == 0) ? Order.CompareTo(other.Order) : fileComp;
        }
    }
}
