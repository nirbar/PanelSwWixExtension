using WixToolset.Data;

namespace PanelSw.Wix.Extensions
{
    public static class PanelSwWixErrorMessages
    {
        public static Message ExecuteCommandSequence(SourceLineNumber sourceLineNumber, string executeCommandId, string otherActionId)
        {
            return new Message(sourceLineNumber, MessageLevel.Error, (int)PswErrorId.ExecuteCommandSequence, MessageResources.ExecuteCommandSequence, executeCommandId, otherActionId);
        }
    }

    public enum PswErrorId
    {
        ExecuteCommandSequence = 9000,
    }
}
