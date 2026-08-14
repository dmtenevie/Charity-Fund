#include "AppIcon.h"
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QPen>
#include <algorithm>

QIcon makeGlyphIcon(const QString& glyph, const QColor& background, int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    painter.setBrush(background);
    painter.drawEllipse(0, 0, size, size);

    QFont font = painter.font();
    font.setPixelSize(static_cast<int>(size * 0.55));
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, glyph);

    return QIcon(pixmap);
}

QIcon makeWindowIcon(WinGlyph glyph, const QColor& color, int size) {
    // Render at 4x and let Qt downscale — crisp on both 100% and HiDPI
    // displays, and avoids the sub-pixel wobble a 1:1 draw gets at this
    // small a size.
    const int superSample = 4;
    const int px = size * superSample;

    QPixmap pixmap(px, px);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(color);
    pen.setWidthF(std::max(1.6, size * 0.11) * superSample);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::MiterJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    const qreal m = px * 0.30; // inset margin from the pixmap edges
    const QRectF box(m, m, px - 2 * m, px - 2 * m);

    switch (glyph) {
    case WinGlyph::Minimize: {
        qreal y = box.center().y();
        painter.drawLine(QPointF(box.left(), y), QPointF(box.right(), y));
        break;
    }
    case WinGlyph::Maximize: {
        painter.drawRect(box);
        break;
    }
    case WinGlyph::Restore: {
        qreal off = box.width() * 0.24;
        QRectF back(box.left() + off, box.top(), box.width() - off, box.height() - off);
        QRectF front(box.left(), box.top() + off, box.width() - off, box.height() - off);
        painter.drawRect(back);
        painter.drawRect(front);
        break;
    }
    case WinGlyph::Close: {
        painter.drawLine(box.topLeft(), box.bottomRight());
        painter.drawLine(box.topRight(), box.bottomLeft());
        break;
    }
    case WinGlyph::Overflow: {
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        qreal r = px * 0.06;
        qreal y = px * 0.5;
        for (qreal x : {px * 0.28, px * 0.5, px * 0.72}) {
            painter.drawEllipse(QPointF(x, y), r, r);
        }
        break;
    }
    }

    QPixmap scaled = pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(1.0);
    return QIcon(scaled);
}
