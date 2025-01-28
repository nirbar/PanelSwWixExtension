using Microsoft.Tools.WindowsInstallerXml;
using System.Resources;

namespace PanelSw.Wix.Extensions
{
    public class PanelSwWixErrorMessages : WixErrorEventArgs
    {
        private PanelSwWixErrorMessages(SourceLineNumberCollection sourceLineNumber, int id, string resourceName, params object[] messageArgs)
            : base(sourceLineNumber, id, resourceName, messageArgs)
        {
            Level = MessageLevel.Error;
        }

        override public ResourceManager ResourceManager => MessageResources.ResourceManager;

        public static PanelSwWixErrorMessages ExecuteCommandSequence(SourceLineNumberCollection sourceLineNumber, string executeCommandId, string otherActionId)
        {
            return new PanelSwWixErrorMessages(sourceLineNumber, (int)PswErrorId.ExecuteCommandSequence, nameof(MessageResources.ExecuteCommandSequence), executeCommandId, otherActionId);
        }

        public static PanelSwWixErrorMessages MismatchingRemoveFolderExLongPathHandling(SourceLineNumberCollection sourceLineNumber)
        {
            return new PanelSwWixErrorMessages(sourceLineNumber, (int)PswErrorId.MismatchingRemoveFolderExLongPathHandling, nameof(MessageResources.MismatchingRemoveFolderExLongPathHandling));
        }
    }

    public enum PswErrorId
    {
        ExecuteCommandSequence = 9000,
        MismatchingRemoveFolderExLongPathHandling,
    }
}
