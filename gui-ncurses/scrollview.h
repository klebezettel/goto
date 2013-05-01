#ifndef SCROLLVIEW_H
#define SCROLLVIEW_H

namespace TUI {
namespace NCurses {

/// Represents the visible lines if there are more menu entries than window lines.
class ScrollView
{
public:
    ScrollView(unsigned firstRow, unsigned rowCount)
        : m_firstRow(firstRow), m_rowCount(rowCount) {}

    unsigned rowCount() { return m_rowCount; }

    unsigned firstRow() { return m_firstRow; }
    unsigned lastRow() { return m_firstRow + m_rowCount - 1; }

    bool isRowBefore(unsigned row) { return row < m_firstRow; }
    bool isRowBehind(unsigned row) { return lastRow() < row; }

    void resetTo(unsigned firstRow) { m_firstRow = firstRow; }

    void moveDown(unsigned times = 1) { while (times--) ++m_firstRow; }
    void moveUp(unsigned times = 1) { while (times--) --m_firstRow; }

private:
    unsigned m_firstRow;
    unsigned m_rowCount;
};

} // namespace NCurses
} // namespace TUI

#endif // SCROLLVIEW_H
