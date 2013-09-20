#ifndef SKINFACTORY_H
#define SKINFACTORY_H

#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QMetaType>


struct Skin {
    QString m_baseName;
    QString m_visibleName;
    QStringList m_stylesNames;
    QString m_author;
    QString m_email;
    QString m_version;
    QString m_rawData;
};

Q_DECLARE_METATYPE(Skin)

class SkinFactory : public QObject {
    Q_OBJECT

  private:
    explicit SkinFactory(QObject *parent = 0);

    // Loads the skin from give skin_data.
    // NOTE: Extra relative path escaping is done for loading of
    // external resources.
    bool loadSkinFromData(QString skin_data, const QString &skin_path);

  public:
    // Singleton getter.
    static SkinFactory *getInstance();

    // Destructor.
    virtual ~SkinFactory();

    // Loads skin name from settings and sets it as active.
    void loadCurrentSkin();

    // Return the name of the currently activated skin.
    // NOTE: Skin name is formatted as "<folder>/<skin>.xml".
    QString getCurrentSkinName();

    Skin getSkinInfo(const QString &skin_name, bool *ok = NULL);

    QList<Skin> getInstalledSkins();

    // Sets the desired skin as the active one if it exists.
    void setCurrentSkinName(const QString &skin_name);

  private:
    // Holds name of the current skin.
    QString m_currentSkin;

    // Singleton.
    static QPointer<SkinFactory> s_instance;
};

#endif // SKINFACTORY_H
