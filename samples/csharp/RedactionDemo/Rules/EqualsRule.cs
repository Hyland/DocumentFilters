internal class EqualsRule : RegExRule
{
    public EqualsRule(string text)
        : base($"\\b{text}\\b")
    {
    }
}
