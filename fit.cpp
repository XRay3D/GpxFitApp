#include "fit.h"

#include <QAbstractTableModel>
#include <QDebug>
#include <QElapsedTimer>
#include <QQmlFile>
#include <QTextStream>
#include <QUrl>
// #include <format>
#include <any>
#include <fstream>
#include <set>
// #include <iostream>

#include <boost/pfr.hpp>
#include <strstream>
namespace pfr = boost::pfr;

#include "fit.h"
#include "fit_decode.hpp"
#include "fit_developer_field_description.hpp"
#include "fit_mesg_broadcaster.hpp"

const double k = (180. / pow(2, 31));
constexpr double m_s_to_km_h = 3.6;

struct Record {
    // Record() { }

    // uint16_t altitude;          // native: 193.000000
    // uint8_t cadence;            // native: 57.000000
    // uint32_t distance;          // native: 415.990000
    // uint32_t enhanced_altitude; // native: 193.000000
    // uint32_t enhanced_speed;    // native: 6.250000
    // int16_t grade;              // native: -0.300000
    // uint8_t heart_rate;         // native: 115.000000
    // int32_t position_lat;       // native: 680525518.000000
    // int32_t position_long;      // native: 417299976.000000
    // uint16_t speed;             // native: 6.250000
    // int8_t temperature;         // native: 22.000000
    // uint32_t timestamp;         // native: 1060153274.000000

    double altitude; // 0  native: 193.000000
    double cadence;  // 1  native: 57.000000
    double distance; // 2  native: 415.990000
    // double enhanced_altitude; // 3  native: 193.000000
    // double enhanced_speed;    // 4  native: 6.250000
    double grade;         // 5  native: -0.300000
    double heart_rate;    // 6  native: 115.000000
    double position_lat;  // 7  native: 680525518.000000
    double position_long; // 8  native: 417299976.000000
    double speed;         // 9  native: 6.250000
    double temperature;   // 10 native: 22.000000
    double timestamp;     // 11 native: 1060153274.000000

    void setField(std::string_view name, double val) {
        [this]<size_t... Is>(std::string_view name, double val, std::index_sequence<Is...>) {
            ((pfr::get_name<Is, Record>() == name ? pfr::get<Is>(*this) = val : val), ...);
        }(name, val, std::make_index_sequence<pfr::tuple_size_v<Record>>{});
    }
};

QString str;
QTextStream ts{&str};

std::vector<Record> Records;

int Model::rowCount(const QModelIndex& parent) const {
    return Records.size();
}

int Model::columnCount(const QModelIndex& parent) const {
    return pfr::tuple_size_v<Record>;
}

QVariant Model::data(const QModelIndex& index, int role) const {
    if(role == Qt::DisplayRole || role == Qt::EditRole)
        return []<size_t... Is>(const Record& rec, int column, std::index_sequence<Is...>) -> QVariant {
            static QDateTime add{
                QDate{1989, 12, 31},
                QTime{}
            };
            switch(column) {
                // clang-format off
            case __COUNTER__ /*0*/:    return QString::number(rec.altitude); // 0  native: 193.000000
            case __COUNTER__ /*1*/:    return QString::number(rec.cadence); // 1  native: 57.000000
            case __COUNTER__ /*2*/:    return QString::number(rec.distance / 1000); // 2  native: 415.990000
            // case __COUNTER__ /*3*/: return QString::number(rec.enhanced_altitude);                                 // 3  native: 193.000000
            // case __COUNTER__ /*4*/: return QString::number(rec.enhanced_speed);                                    // 4  native: 6.250000
            case __COUNTER__ /*5*/:    return QString::number(rec.grade); // 5  native: -0.300000
            case __COUNTER__ /*6*/:    return QString::number(rec.heart_rate); // 6  native: 115.000000
            case __COUNTER__ /*7*/:    return QString::number(rec.position_lat * k); // 7  native: 680525518.000000
            case __COUNTER__ /*8*/:    return QString::number(rec.position_long * k); // 8  native: 417299976.000000
            case __COUNTER__ /*9*/:    return QString::number(rec.speed * m_s_to_km_h); // 9  native: 6.250000
            case __COUNTER__ /*10*/:   return QString::number(rec.temperature); // 10 native: 22.000000
            case __COUNTER__ /*11*/:   return QDateTime::fromSecsSinceEpoch(rec.timestamp + add.toSecsSinceEpoch()); // 11 native: 1060153274.000000
                // clang-format on
            }
            return {};
            // if(column == pfr::tuple_size_v<Record> - 1) {
            //     QDateTime add{
            //         QDate{1980, 1, 6},
            //         {}
            //     };
            //     // January 6, 1980, 00:00:00
            //     return QDateTime::fromSecsSinceEpoch(rec.timestamp + add.toSecsSinceEpoch());
            // }
            // QVariant var;
            // ((Is == column ? var = pfr::get<Is>(rec) : var), ...);
            // return var;
        }(Records[index.row()], index.column(), std::make_index_sequence<pfr::tuple_size_v<Record>>{});
    return {};
}

QHash<int, QByteArray> Model::roleNames() const {
    return QAbstractTableModel::roleNames();
    // return {
    //     {Qt::DisplayRole, "display"}
    // };
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const {
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return [this]<size_t... Is>(int section, std::index_sequence<Is...>) -> QVariant {
            static constexpr std::array names{pfr::get_name<Is, Record>()...};
            return names[section].data();
        }(section, std::make_index_sequence<pfr::tuple_size_v<Record>>{});
    return QAbstractTableModel::headerData(section, orientation, role);
}

QList<QGeoCoordinate> Model::route() const {
    QList<QGeoCoordinate> ret;
    ret.reserve(Records.size());
    for(auto&& rec: Records) {
        QGeoCoordinate geo{rec.position_lat * k, rec.position_long * k, rec.altitude};
        if(rec.position_lat && rec.position_long && geo.isValid())
            ret.push_back(geo);
    }
    return ret;
}

// #define printf_(...)
template <typename... Ts>
void printf_(Ts&&... vals) {
    char data[1024]{};
    sprintf(data, vals...);
    ts << data;
}

class Listener final : public fit::FileIdMesgListener,
                       public fit::UserProfileMesgListener,
                       public fit::MonitoringMesgListener,
                       public fit::DeviceInfoMesgListener,
                       public fit::MesgListener,
                       public fit::DeveloperFieldDescriptionListener,
                       public fit::RecordMesgListener {
public:
    static void PrintValue(const fit::FieldBase& field) { }

    static void PrintValues(const fit::FieldBase& field) {
        for(FIT_UINT8 j = 0; j < (FIT_UINT8)field.GetNumValues(); j++) {
            ts << "       Va" << j << ": ";
            switch(field.GetType()) {
            // Get float 64 values for numeric types to receive values that have
            // their scale and offset properly applied.
            case FIT_BASE_TYPE_ENUM:    // ts << field.GetENUMValue(j); break;
            case FIT_BASE_TYPE_BYTE:    // ts << field.GetBYTEValue(j); break;
            case FIT_BASE_TYPE_SINT8:   // ts << field.GetSINT8Value(j); break;
            case FIT_BASE_TYPE_UINT8:   // ts << field.GetUINT8Value(j); break;
            case FIT_BASE_TYPE_SINT16:  // ts << field.GetSINT16Value(j); break;
            case FIT_BASE_TYPE_UINT16:  // ts << field.GetUINT16Value(j); break;
            case FIT_BASE_TYPE_SINT32:  // ts << field.GetSINT32Value(j); break;
            case FIT_BASE_TYPE_UINT32:  // ts << field.GetUINT32Value(j); break;
            case FIT_BASE_TYPE_SINT64:  // ts << field.GetSINT64Value(j); break;
            case FIT_BASE_TYPE_UINT64:  // ts << field.GetUINT64Value(j); break;
            case FIT_BASE_TYPE_UINT8Z:  // ts << field.GetUINT8ZValue(j); break;
            case FIT_BASE_TYPE_UINT16Z: // ts << field.GetUINT16ZValue(j); break;
            case FIT_BASE_TYPE_UINT32Z: // ts << field.GetUINT32ZValue(j); break;
            case FIT_BASE_TYPE_UINT64Z: // ts << field.GetUINT64ZValue(j); break;
            case FIT_BASE_TYPE_FLOAT32: // ts << field.GetFLOAT32Value(j); break;
            case FIT_BASE_TYPE_FLOAT64:
                ts << field.GetFLOAT64Value(j);
                break;
            case FIT_BASE_TYPE_STRING:
                ts << field.GetSTRINGValue(j).c_str();
                break;
            default:
                break;
            }
            ts << " " << field.GetUnits().c_str() << "\n";
        }
    }

    void OnMesg(fit::Mesg& mesg) override {
        printf_("%s", "On Mesg:\n");
        ts << "   New Mesg: " << mesg.GetName().c_str() << ".  It has " << mesg.GetNumFields()
           << " field(s) and " << mesg.GetNumDevFields() << " developer field(s).\n";

        for(FIT_UINT16 i = 0; i < (FIT_UINT16)mesg.GetNumFields(); i++) {
            fit::Field* field = mesg.GetFieldByIndex(i);
            ts << "   Field" << i << " (" << field->GetName().c_str() << ") has "
               << field->GetNumValues() << " value(s)\n";
            PrintValues(*field);
        }

        for(auto devField: mesg.GetDeveloperFields()) {
            ts << "   Developer Field(" << devField.GetName().c_str() << ") has "
               << devField.GetNumValues() << " value(s)\n";
            PrintValues(devField);
        }
    }

    void OnMesg(fit::FileIdMesg& mesg) override {
        printf_("%s", "File ID:\n");
        if(mesg.IsTypeValid())
            printf_("   Type: %d\n", mesg.GetType());
        if(mesg.IsManufacturerValid())
            printf_("   Manufacturer: %d\n", mesg.GetManufacturer());
        if(mesg.IsProductValid())
            printf_("   Product: %d\n", mesg.GetProduct());
        if(mesg.IsSerialNumberValid())
            printf_("   Serial Number: %u\n", mesg.GetSerialNumber());
        if(mesg.IsNumberValid())
            printf_("   Number: %d\n", mesg.GetNumber());
    }

    void OnMesg(fit::UserProfileMesg& mesg) override {
        printf_("%s", "User profile:\n");
        if(mesg.IsFriendlyNameValid())
            ts << "   Friendly Name: " << mesg.GetFriendlyName().c_str() << "\n";
        if(mesg.GetGender() == FIT_GENDER_MALE)
            printf_("%s", "   Gender: Male\n");
        if(mesg.GetGender() == FIT_GENDER_FEMALE)
            printf_("%s", "   Gender: Female\n");
        if(mesg.IsAgeValid())
            printf_("   Age [years]: %d\n", mesg.GetAge());
        if(mesg.IsWeightValid())
            printf_("   Weight [kg]: %0.2f\n", mesg.GetWeight());
    }

    void OnMesg(fit::DeviceInfoMesg& mesg) override {
        printf_("%s", "Device info:\n");

        if(mesg.IsTimestampValid())
            printf_("   Timestamp: %d\n", mesg.GetTimestamp());

        switch(mesg.GetBatteryStatus()) {
        case FIT_BATTERY_STATUS_CRITICAL:
            printf_("%s", "   Battery status: Critical\n");
            return;
        case FIT_BATTERY_STATUS_GOOD:
            printf_("%s", "   Battery status: Good\n");
            return;
        case FIT_BATTERY_STATUS_LOW:
            printf_("%s", "   Battery status: Low\n");
            return;
        case FIT_BATTERY_STATUS_NEW:
            printf_("%s", "   Battery status: New\n");
            return;
        case FIT_BATTERY_STATUS_OK:
            printf_("%s", "   Battery status: OK\n");
            return;
        default:
            printf_("%s", "   Battery status: Invalid\n");
            return;
        }
    }

    void OnMesg(fit::MonitoringMesg& mesg) override {
        printf_("%s", "Monitoring:\n");

        if(mesg.IsTimestampValid())
            printf_("   Timestamp: %d\n", mesg.GetTimestamp());

        if(mesg.IsActivityTypeValid())
            printf_("   Activity type: %d\n", mesg.GetActivityType());

        switch(mesg.GetActivityType()) // The Cycling field is dynamic
        {
        case FIT_ACTIVITY_TYPE_WALKING:
        case FIT_ACTIVITY_TYPE_RUNNING: // Intentional fallthrough
            if(mesg.IsStepsValid())
                printf_("   Steps: %d\n", mesg.GetSteps());
            return;
        case FIT_ACTIVITY_TYPE_CYCLING:
        case FIT_ACTIVITY_TYPE_SWIMMING: // Intentional fallthrough
            if(mesg.IsStrokesValid())
                printf_("Strokes: %f\n", mesg.GetStrokes());
            return;
        default:
            if(mesg.IsCyclesValid())
                printf_("Cycles: %f\n", mesg.GetCycles());
            return;
        }
    }

    static void PrintOverrideValues(const fit::Mesg& mesg, FIT_UINT8 fieldNum) {
        std::vector<const fit::FieldBase*> fields = mesg.GetOverrideFields(fieldNum);
        const fit::Profile::FIELD* profileField = fit::Profile::GetField(mesg.GetNum(), fieldNum);
        FIT_BOOL namePrinted = FIT_FALSE;

        for(const fit::FieldBase* field: fields) {
            if(!namePrinted) {
                printf_("   %s:\n", profileField->name.c_str());
                namePrinted = FIT_TRUE;
            }

            if(FIT_NULL != dynamic_cast<const fit::Field*>(field)) {
                // Native Field
                printf_("%s", "      native: ");
            } else {
                // Developer Field
                printf_("%s", "      override: ");
            }
#if 0
            switch(field->GetType()) {
            // Get float 64 values for numeric types to receive values that have
            // their scale and offset properly applied.
            case FIT_BASE_TYPE_ENUM: printf_("ENUM "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_BYTE: printf_("BYTE "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_SINT8: printf_("SINT8 "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_UINT8: printf_("UINT8 "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_SINT16: printf_("SINT16 "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_UINT16: printf_("UINT16 "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_SINT32: printf_("SINT32 "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_UINT32: printf_("UINT32 "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_SINT64: printf_("SINT64 "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_UINT64: printf_("UINT64 "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_UINT8Z: printf_("UINT8Z "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_UINT16Z: printf_("UINT16Z "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_UINT32Z: printf_("UINT32Z "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_UINT64Z: printf_("UINT64Z "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_FLOAT32: printf_("FLOAT32 "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_FLOAT64: printf_("FLOAT64 "), printf_("%f\n", field->GetFLOAT64Value()); break;
            case FIT_BASE_TYPE_STRING: printf_("STRING "), printf_("%ls\n", field->GetSTRINGValue().c_str()); break;
            default: break;
            }
#else
            switch(field->GetType()) {
            // Get float 64 values for numeric types to receive values that have
            // their scale and offset properly applied.
            case FIT_BASE_TYPE_ENUM:
            case FIT_BASE_TYPE_BYTE:
            case FIT_BASE_TYPE_SINT8:
            case FIT_BASE_TYPE_UINT8:
            case FIT_BASE_TYPE_SINT16:
            case FIT_BASE_TYPE_UINT16:
            case FIT_BASE_TYPE_SINT32:
            case FIT_BASE_TYPE_UINT32:
            case FIT_BASE_TYPE_SINT64:
            case FIT_BASE_TYPE_UINT64:
            case FIT_BASE_TYPE_UINT8Z:
            case FIT_BASE_TYPE_UINT16Z:
            case FIT_BASE_TYPE_UINT32Z:
            case FIT_BASE_TYPE_UINT64Z:
            case FIT_BASE_TYPE_FLOAT32:
            case FIT_BASE_TYPE_FLOAT64:
                printf_("%f %s\n",
                    field->GetFLOAT64Value() /*/ profileField->scale*/,
                    profileField->units.c_str());
                Records.back().setField(profileField->name,
                    field->GetFLOAT64Value() /*/ profileField->scale*/);
                break;
            case FIT_BASE_TYPE_STRING:
                printf_("%ls\n", field->GetSTRINGValue().c_str());
                break;
            default:
                break;
            }
#endif
        }
    }

    void OnMesg(fit::RecordMesg& record) override {
        printf_("%s", "Record:\n");
        if(Records.empty())
            Records.reserve(10000000);
        // if(Records.size() > 100) return;
        Records.emplace_back(Record{});

        PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Altitude);
        PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Cadence);
        PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Distance);
        PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::EnhancedAltitude);
        PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::EnhancedSpeed);
        PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Grade);
        PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::HeartRate);
        PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::PositionLat);
        PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::PositionLong);
        PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Speed);
        PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Temperature);
        PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Timestamp);

        // PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::HeartRate);
        // PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Cadence);
        // PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Distance);
        // PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Speed);
    }

    void OnDeveloperFieldDescription(const fit::DeveloperFieldDescription& desc) override {
        printf_("%s", "New Developer Field Description\n");
        printf_("   App Version: %d\n", desc.GetApplicationVersion());
        printf_("   Field Number: %d\n", desc.GetFieldDefinitionNumber());
    }
};

Fit::Fit(QObject* parent)
    : QObject{parent} { }

static QString getRealPathFromUri(const QUrl& url) {
    QString path = "";

    QFileInfo info = QFileInfo(url.toString());
    if(info.isFile()) {
        QString abs = QFileInfo(url.toString()).absoluteFilePath();
        if(!abs.isEmpty() && abs != url.toString() && QFileInfo(abs).isFile())
            return abs;
    } else if(info.isDir()) {
        QString abs = QFileInfo(url.toString()).absolutePath();
        if(!abs.isEmpty() && abs != url.toString() && QFileInfo(abs).isDir())
            return abs;
    }
    QString localfile = url.toLocalFile();
    if((QFileInfo(localfile).isFile() || QFileInfo(localfile).isDir()) && localfile != url.toString())
        return localfile;
#ifdef Q_OS_ANDROID
    QJniObject jUrl = QJniObject::fromString(url.toString());
    QJniObject jContext = QtAndroidPrivate::context();
    QJniObject jContentResolver = jContext.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
    QJniObject jUri = QJniObject::callStaticObjectMethod("android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;", jUrl.object<jstring>());
    QJniObject jCursor = jContentResolver.callObjectMethod("query", "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;", jUri.object<jobject>(), nullptr, nullptr, nullptr, nullptr);
    QJniObject jScheme = jUri.callObjectMethod("getScheme", "()Ljava/lang/String;");
    QJniObject authority;
    if(jScheme.isValid())
        authority = jUri.callObjectMethod("getAuthority", "()Ljava/lang/String;");
    if(authority.isValid() && authority.toString() == "com.android.externalstorage.documents") {
        QJniObject jPath = jUri.callObjectMethod("getPath", "()Ljava/lang/String;");
        path = jPath.toString();
    } else if(jCursor.isValid() && jCursor.callMethod<jboolean>("moveToFirst")) {
        QJniObject jColumnIndex = QJniObject::fromString("_data");
        jint columnIndex = jCursor.callMethod<jint>("getColumnIndexOrThrow", "(Ljava/lang/String;)I", jColumnIndex.object<jstring>());
        QJniObject jRealPath = jCursor.callObjectMethod("getString", "(I)Ljava/lang/String;", columnIndex);
        path = jRealPath.toString();
        if(authority.isValid() && authority.toString().startsWith("com.android.providers") && !url.toString().startsWith("content://media/external/")) {
            QStringList list = path.split(":");
            if(list.count() == 2) {
                QString type = list.at(0);
                QString id = list.at(1);
                if(type == "image")
                    type = type + "s";
                if(type == "document" || type == "documents")
                    type = "file";
                if(type == "msf")
                    type = "downloads";
                if(QList<QString>({"images", "video", "audio"}).contains(type))
                    type = type + "/media";
                path = "content://media/external/" + type;
                path = path + "/" + id;
                return getRealPathFromUri(path);
            }
        }
    } else {
        QJniObject jPath = jUri.callObjectMethod("getPath", "()Ljava/lang/String;");
        path = jPath.toString();
        qDebug() << QFile::exists(path) << path;
    }

    if(path.startsWith("primary:")) {
        path = path.remove(0, QString("primary:").length());
        path = "/sdcard/" + path;
    } else if(path.startsWith("/document/primary:")) {
        path = path.remove(0, QString("/document/primary:").length());
        path = "/sdcard/" + path;
    } else if(path.startsWith("/tree/primary:")) {
        path = path.remove(0, QString("/tree/primary:").length());
        path = "/sdcard/" + path;
    } else if(path.startsWith("/storage/emulated/0/")) {
        path = path.remove(0, QString("/storage/emulated/0/").length());
        path = "/sdcard/" + path;
    } else if(path.startsWith("/tree//")) {
        path = path.remove(0, QString("/tree//").length());
        path = "/" + path;
    }
    if(!QFileInfo(path).isFile() && !QFileInfo(path).isDir() && !path.startsWith("/data"))
        return url.toString();
    return path;
#else
    return url.toString();
#endif
}

bool Fit::loadFile(const QString& filePath) {
    QElapsedTimer timer;
    timer.start();

    fit::Decode decode;
    // decode.SkipHeader();       // Use on streams with no header and footer (stream contains FIT defn and data messages only)
    // decode.IncompleteStream(); // This suppresses exceptions with unexpected eof (also incorrect crc)
    fit::MesgBroadcaster mesgBroadcaster;
    Listener listener;

    qInfo("FIT Decode Example Application\n");
    qInfo() << filePath;
#ifdef Q_OS_ANDROID
    // auto argv = '/' + filePath.split("%3A%2F").back().replace("%2F", "/").toStdString();
    // qInfo() << QDir{}.exists(filePath);
    // qInfo() << QDir{}.exists(argv.c_str());

    // QFileInfo fi{filePath};

    // qCritical() << "absoluteFilePath" << fi.absoluteFilePath();
    // qCritical() << "absolutePath" << fi.absolutePath();
    // qCritical() << "baseName" << fi.baseName();
    // qCritical() << "bundleName" << fi.bundleName();
    // qCritical() << "canonicalFilePath" << fi.canonicalFilePath();
    // qCritical() << "canonicalPath" << fi.canonicalPath();
    // qCritical() << "completeBaseName" << fi.completeBaseName();
    // qCritical() << "completeSuffix" << fi.completeSuffix();
    // qCritical() << "fileName" << fi.fileName();
    // qCritical() << "filePath" << fi.filePath();
    // qCritical() << "group" << fi.group();
    // qCritical() << "junctionTarget" << fi.junctionTarget();
    // qCritical() << "owner" << fi.owner();
    // qCritical() << "path" << fi.path();
    // qCritical() << "readSymLink" << fi.readSymLink();
    // qCritical() << "suffix" << fi.suffix();
    // qCritical() << "symLinkTarget" << fi.symLinkTarget();

    // qCritical() << "symLinkTarget" << fi.dir();
    // qCritical() << "symLinkTarget" << fi.absoluteDir();

    // qCritical() << "filesystemAbsoluteFilePath" << fi.filesystemAbsoluteFilePath().c_str();
    // qCritical() << "filesystemAbsolutePath" << fi.filesystemAbsolutePath().c_str();
    // qCritical() << "filesystemCanonicalFilePath" << fi.filesystemCanonicalFilePath().c_str();
    // qCritical() << "filesystemCanonicalPath" << fi.filesystemCanonicalPath().c_str();
    // qCritical() << "filesystemFilePath" << fi.filesystemFilePath().c_str();
    // qCritical() << "filesystemJunctionTarget" << fi.filesystemJunctionTarget().c_str();
    // qCritical() << "filesystemPath" << fi.filesystemPath().c_str();
    // qCritical() << "filesystemReadSymLink" << fi.filesystemReadSymLink().c_str();
    // qCritical() << "filesystemSymLinkTarget" << fi.filesystemSymLinkTarget().c_str();

    // QJniObject uri = QJniObject::callStaticObjectMethod(
    //     "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
    //     QJniObject::fromString(filePath).object<jstring>());

    // QString filename = QJniObject::callStaticObjectMethod(
    //     "br/com/myjavapackage/PathUtil", "getFileName",
    //     "(Landroid/net/Uri;Landroid/content/Context;)Ljava/lang/String;",
    //     uri.object() /*, QtAndroid::androidContext().object()*/)
    //                        .toString();
    QByteArray data;
    if(QFile file{filePath}; file.open(QFile::ReadOnly))
        data = file.readAll();

    // struct OneShotReadBuf : public std::streambuf {
        // OneShotReadBuf(char* s, std::size_t n) {
            // setg(s, s, s + n);
        // }
    // };
    // OneShotReadBuf osrb(data.data(), data.size());

    std::istrstream file{data.data(), data.size()};
    // std::istream file{&osrb};

    file.seekg(0, file.end);
    qWarning() << file.tellg();
    file.seekg(0, file.beg);

#else
    std::fstream file;
    auto argv = filePath.toStdString();
    file.open(argv, std::ios::in | std::ios::binary);
    if(!file.is_open()) {
        qWarning("Error opening file %s\n", argv.c_str());
        int error = errno;
        const char* errorMessage = strerror(error);
        qWarning() << "Error opening file: " << errorMessage;
    }
#endif

    if(!decode.CheckIntegrity(file))
        qWarning("FIT file integrity failed.\nAttempting to decode...\n");

    // mesgBroadcaster.AddListener(static_cast<fit::FileIdMesgListener&>(listener));
    mesgBroadcaster.AddListener(static_cast<fit::UserProfileMesgListener&>(listener));
    // mesgBroadcaster.AddListener(static_cast<fit::MonitoringMesgListener&>(listener));
    // mesgBroadcaster.AddListener(static_cast<fit::DeviceInfoMesgListener&>(listener));
    mesgBroadcaster.AddListener(static_cast<fit::RecordMesgListener&>(listener));
    // mesgBroadcaster.AddListener(static_cast<fit::MesgListener&>(listener));

    try {
        decode.Read(&file, &mesgBroadcaster, &mesgBroadcaster, &listener);
    } catch(const fit::RuntimeException& e) {
        qCritical("Exception decoding file: %s\n", e.what());
        return -1;
    }

    Records.shrink_to_fit();

    str.resize(100000);
    qDebug() << str.toLocal8Bit().data();

    // qInfo("Decoded FIT file %s.\n", argv.c_str());

    qCritical() << "elapsed" << timer.elapsed() << "ms";
    // exit(-99);
    // str.resize(1000);
    setText(str);

    return 0;
}

// ////////////////////////////////////////
// #include "fit.h"
// // #include "private/qjnihelpers_p.h"

// #include <QAbstractTableModel>
// #include <QDebug>
// #include <QElapsedTimer>
// #include <QQmlFile>
// #include <QTextStream>
// #include <QUrl>
// // #include <format>
// // #include <any>
// #include <fstream>
// // #include <set>
// // #include <iostream>

// #include <QJniObject>

// #include <boost/pfr.hpp>
// namespace pfr = boost::pfr;

// #include "fit.h"
// #include "fit_decode.hpp"
// #include "fit_developer_field_description.hpp"
// #include "fit_mesg_broadcaster.hpp"

// const double k = (180. / pow(2, 31));
// constexpr double m_s_to_km_h = 3.6;

// struct Record {
//     // Record() { }

//            // uint16_t altitude;          // native: 193.000000
//            // uint8_t cadence;            // native: 57.000000
//            // uint32_t distance;          // native: 415.990000
//            // uint32_t enhanced_altitude; // native: 193.000000
//            // uint32_t enhanced_speed;    // native: 6.250000
//            // int16_t grade;              // native: -0.300000
//            // uint8_t heart_rate;         // native: 115.000000
//            // int32_t position_lat;       // native: 680525518.000000
//            // int32_t position_long;      // native: 417299976.000000
//            // uint16_t speed;             // native: 6.250000
//            // int8_t temperature;         // native: 22.000000
//            // uint32_t timestamp;         // native: 1060153274.000000

//     double altitude; // 0  native: 193.000000
//     double cadence;  // 1  native: 57.000000
//     double distance; // 2  native: 415.990000
//     // double enhanced_altitude; // 3  native: 193.000000
//     // double enhanced_speed;    // 4  native: 6.250000
//     double grade;         // 5  native: -0.300000
//     double heart_rate;    // 6  native: 115.000000
//     double position_lat;  // 7  native: 680525518.000000
//     double position_long; // 8  native: 417299976.000000
//     double speed;         // 9  native: 6.250000
//     double temperature;   // 10 native: 22.000000
//     double timestamp;     // 11 native: 1060153274.000000

//     void setField(std::string_view name, double val) {
//         [this]<size_t... Is>(std::string_view name, double val, std::index_sequence<Is...>) {
//             ((pfr::get_name<Is, Record>() == name ? pfr::get<Is>(*this) = val : val), ...);
//         }(name, val, std::make_index_sequence<pfr::tuple_size_v<Record>>{});
//     }
// };

// QString str;
// QTextStream ts{&str};

// std::vector<Record> Records;

// int Model::rowCount(const QModelIndex& parent) const {
//     return Records.size();
// }

// int Model::columnCount(const QModelIndex& parent) const {
//     return pfr::tuple_size_v<Record>;
// }

// QVariant Model::data(const QModelIndex& index, int role) const {
//     if(role == Qt::DisplayRole || role == Qt::EditRole)
//         return []<size_t... Is>(const Record& rec, int column, std::index_sequence<Is...>) -> QVariant {
//             static QDateTime add{
//                 QDate{1989, 12, 31},
//                 QTime{}
//             };
//             switch(column) {
//                 // clang-format off
//             case __COUNTER__ /*0*/:    return QString::number(rec.altitude); // 0  native: 193.000000
//             case __COUNTER__ /*1*/:    return QString::number(rec.cadence); // 1  native: 57.000000
//             case __COUNTER__ /*2*/:    return QString::number(rec.distance / 1000); // 2  native: 415.990000
//             // case __COUNTER__ /*3*/: return QString::number(rec.enhanced_altitude);                                 // 3  native: 193.000000
//             // case __COUNTER__ /*4*/: return QString::number(rec.enhanced_speed);                                    // 4  native: 6.250000
//             case __COUNTER__ /*5*/:    return QString::number(rec.grade); // 5  native: -0.300000
//             case __COUNTER__ /*6*/:    return QString::number(rec.heart_rate); // 6  native: 115.000000
//             case __COUNTER__ /*7*/:    return QString::number(rec.position_lat * k); // 7  native: 680525518.000000
//             case __COUNTER__ /*8*/:    return QString::number(rec.position_long * k); // 8  native: 417299976.000000
//             case __COUNTER__ /*9*/:    return QString::number(rec.speed * m_s_to_km_h); // 9  native: 6.250000
//             case __COUNTER__ /*10*/:   return QString::number(rec.temperature); // 10 native: 22.000000
//             case __COUNTER__ /*11*/:   return QDateTime::fromSecsSinceEpoch(rec.timestamp + add.toSecsSinceEpoch()); // 11 native: 1060153274.000000
//                 // clang-format on
//             }
//             return {};
//             // if(column == pfr::tuple_size_v<Record> - 1) {
//             //     QDateTime add{
//             //         QDate{1980, 1, 6},
//             //         {}
//             //     };
//             //     // January 6, 1980, 00:00:00
//             //     return QDateTime::fromSecsSinceEpoch(rec.timestamp + add.toSecsSinceEpoch());
//             // }
//             // QVariant var;
//             // ((Is == column ? var = pfr::get<Is>(rec) : var), ...);
//             // return var;
//         }(Records[index.row()], index.column(), std::make_index_sequence<pfr::tuple_size_v<Record>>{});
//     return {};
// }

// QHash<int, QByteArray> Model::roleNames() const {
//     return QAbstractTableModel::roleNames();
//     // return {
//     //     {Qt::DisplayRole, "display"}
//     // };
// }

// QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const {
//     if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
//         return [this]<size_t... Is>(int section, std::index_sequence<Is...>) -> QVariant {
//             static constexpr std::array names{pfr::get_name<Is, Record>()...};
//             return names[section].data();
//         }(section, std::make_index_sequence<pfr::tuple_size_v<Record>>{});
//     return QAbstractTableModel::headerData(section, orientation, role);
// }

// QList<QGeoCoordinate> Model::route() const {
//     QList<QGeoCoordinate> ret;
//     ret.reserve(Records.size());
//     for(auto&& rec: Records) {
//         QGeoCoordinate geo{rec.position_lat * k, rec.position_long * k, rec.altitude};
//         if(rec.position_lat && rec.position_long && geo.isValid())
//             ret.push_back(geo);
//     }
//     return ret;
// }

// // #define printf_(...)
// template <typename... Ts>
// void printf_(Ts&&... vals) {
//     char data[1024]{};
//     sprintf(data, vals...);
//     ts << data;
// }

// class Listener final : public fit::FileIdMesgListener,
//                        public fit::UserProfileMesgListener,
//                        public fit::MonitoringMesgListener,
//                        public fit::DeviceInfoMesgListener,
//                        public fit::MesgListener,
//                        public fit::DeveloperFieldDescriptionListener,
//                        public fit::RecordMesgListener {
// public:
//     static void PrintValue(const fit::FieldBase& field) { }

//     static void PrintValues(const fit::FieldBase& field) {
//         for(FIT_UINT8 j = 0; j < (FIT_UINT8)field.GetNumValues(); j++) {
//             ts << "       Va" << j << ": ";
//             switch(field.GetType()) {
//             // Get float 64 values for numeric types to receive values that have
//             // their scale and offset properly applied.
//             case FIT_BASE_TYPE_ENUM:    // ts << field.GetENUMValue(j); break;
//             case FIT_BASE_TYPE_BYTE:    // ts << field.GetBYTEValue(j); break;
//             case FIT_BASE_TYPE_SINT8:   // ts << field.GetSINT8Value(j); break;
//             case FIT_BASE_TYPE_UINT8:   // ts << field.GetUINT8Value(j); break;
//             case FIT_BASE_TYPE_SINT16:  // ts << field.GetSINT16Value(j); break;
//             case FIT_BASE_TYPE_UINT16:  // ts << field.GetUINT16Value(j); break;
//             case FIT_BASE_TYPE_SINT32:  // ts << field.GetSINT32Value(j); break;
//             case FIT_BASE_TYPE_UINT32:  // ts << field.GetUINT32Value(j); break;
//             case FIT_BASE_TYPE_SINT64:  // ts << field.GetSINT64Value(j); break;
//             case FIT_BASE_TYPE_UINT64:  // ts << field.GetUINT64Value(j); break;
//             case FIT_BASE_TYPE_UINT8Z:  // ts << field.GetUINT8ZValue(j); break;
//             case FIT_BASE_TYPE_UINT16Z: // ts << field.GetUINT16ZValue(j); break;
//             case FIT_BASE_TYPE_UINT32Z: // ts << field.GetUINT32ZValue(j); break;
//             case FIT_BASE_TYPE_UINT64Z: // ts << field.GetUINT64ZValue(j); break;
//             case FIT_BASE_TYPE_FLOAT32: // ts << field.GetFLOAT32Value(j); break;
//             case FIT_BASE_TYPE_FLOAT64:
//                 ts << field.GetFLOAT64Value(j);
//                 break;
//             case FIT_BASE_TYPE_STRING:
//                 ts << field.GetSTRINGValue(j).c_str();
//                 break;
//             default:
//                 break;
//             }
//             ts << " " << field.GetUnits().c_str() << "\n";
//         }
//     }

//     void OnMesg(fit::Mesg& mesg) override {
//         printf_("%s", "On Mesg:\n");
//         ts << "   New Mesg: " << mesg.GetName().c_str() << ".  It has " << mesg.GetNumFields()
//            << " field(s) and " << mesg.GetNumDevFields() << " developer field(s).\n";

//         for(FIT_UINT16 i = 0; i < (FIT_UINT16)mesg.GetNumFields(); i++) {
//             fit::Field* field = mesg.GetFieldByIndex(i);
//             ts << "   Field" << i << " (" << field->GetName().c_str() << ") has "
//                << field->GetNumValues() << " value(s)\n";
//             PrintValues(*field);
//         }

//         for(auto devField: mesg.GetDeveloperFields()) {
//             ts << "   Developer Field(" << devField.GetName().c_str() << ") has "
//                << devField.GetNumValues() << " value(s)\n";
//             PrintValues(devField);
//         }
//     }

//     void OnMesg(fit::FileIdMesg& mesg) override {
//         printf_("%s", "File ID:\n");
//         if(mesg.IsTypeValid())
//             printf_("   Type: %d\n", mesg.GetType());
//         if(mesg.IsManufacturerValid())
//             printf_("   Manufacturer: %d\n", mesg.GetManufacturer());
//         if(mesg.IsProductValid())
//             printf_("   Product: %d\n", mesg.GetProduct());
//         if(mesg.IsSerialNumberValid())
//             printf_("   Serial Number: %u\n", mesg.GetSerialNumber());
//         if(mesg.IsNumberValid())
//             printf_("   Number: %d\n", mesg.GetNumber());
//     }

//     void OnMesg(fit::UserProfileMesg& mesg) override {
//         printf_("%s", "User profile:\n");
//         if(mesg.IsFriendlyNameValid())
//             ts << "   Friendly Name: " << mesg.GetFriendlyName().c_str() << "\n";
//         if(mesg.GetGender() == FIT_GENDER_MALE)
//             printf_("%s", "   Gender: Male\n");
//         if(mesg.GetGender() == FIT_GENDER_FEMALE)
//             printf_("%s", "   Gender: Female\n");
//         if(mesg.IsAgeValid())
//             printf_("   Age [years]: %d\n", mesg.GetAge());
//         if(mesg.IsWeightValid())
//             printf_("   Weight [kg]: %0.2f\n", mesg.GetWeight());
//     }

//     void OnMesg(fit::DeviceInfoMesg& mesg) override {
//         printf_("%s", "Device info:\n");

//         if(mesg.IsTimestampValid())
//             printf_("   Timestamp: %d\n", mesg.GetTimestamp());

//         switch(mesg.GetBatteryStatus()) {
//         case FIT_BATTERY_STATUS_CRITICAL:
//             printf_("%s", "   Battery status: Critical\n");
//             return;
//         case FIT_BATTERY_STATUS_GOOD:
//             printf_("%s", "   Battery status: Good\n");
//             return;
//         case FIT_BATTERY_STATUS_LOW:
//             printf_("%s", "   Battery status: Low\n");
//             return;
//         case FIT_BATTERY_STATUS_NEW:
//             printf_("%s", "   Battery status: New\n");
//             return;
//         case FIT_BATTERY_STATUS_OK:
//             printf_("%s", "   Battery status: OK\n");
//             return;
//         default:
//             printf_("%s", "   Battery status: Invalid\n");
//             return;
//         }
//     }

//     void OnMesg(fit::MonitoringMesg& mesg) override {
//         printf_("%s", "Monitoring:\n");

//         if(mesg.IsTimestampValid())
//             printf_("   Timestamp: %d\n", mesg.GetTimestamp());

//         if(mesg.IsActivityTypeValid())
//             printf_("   Activity type: %d\n", mesg.GetActivityType());

//         switch(mesg.GetActivityType()) // The Cycling field is dynamic
//         {
//         case FIT_ACTIVITY_TYPE_WALKING:
//         case FIT_ACTIVITY_TYPE_RUNNING: // Intentional fallthrough
//             if(mesg.IsStepsValid())
//                 printf_("   Steps: %d\n", mesg.GetSteps());
//             return;
//         case FIT_ACTIVITY_TYPE_CYCLING:
//         case FIT_ACTIVITY_TYPE_SWIMMING: // Intentional fallthrough
//             if(mesg.IsStrokesValid())
//                 printf_("Strokes: %f\n", mesg.GetStrokes());
//             return;
//         default:
//             if(mesg.IsCyclesValid())
//                 printf_("Cycles: %f\n", mesg.GetCycles());
//             return;
//         }
//     }

//     static void PrintOverrideValues(const fit::Mesg& mesg, FIT_UINT8 fieldNum) {
//         std::vector<const fit::FieldBase*> fields = mesg.GetOverrideFields(fieldNum);
//         const fit::Profile::FIELD* profileField = fit::Profile::GetField(mesg.GetNum(), fieldNum);
//         FIT_BOOL namePrinted = FIT_FALSE;

//         for(const fit::FieldBase* field: fields) {
//             if(!namePrinted) {
//                 printf_("   %s:\n", profileField->name.c_str());
//                 namePrinted = FIT_TRUE;
//             }

//             if(FIT_NULL != dynamic_cast<const fit::Field*>(field)) {
//                 // Native Field
//                 printf_("%s", "      native: ");
//             } else {
//                 // Developer Field
//                 printf_("%s", "      override: ");
//             }
// #if 0
//             switch(field->GetType()) {
//             // Get float 64 values for numeric types to receive values that have
//             // their scale and offset properly applied.
//             case FIT_BASE_TYPE_ENUM: printf_("ENUM "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_BYTE: printf_("BYTE "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_SINT8: printf_("SINT8 "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_UINT8: printf_("UINT8 "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_SINT16: printf_("SINT16 "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_UINT16: printf_("UINT16 "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_SINT32: printf_("SINT32 "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_UINT32: printf_("UINT32 "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_SINT64: printf_("SINT64 "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_UINT64: printf_("UINT64 "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_UINT8Z: printf_("UINT8Z "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_UINT16Z: printf_("UINT16Z "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_UINT32Z: printf_("UINT32Z "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_UINT64Z: printf_("UINT64Z "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_FLOAT32: printf_("FLOAT32 "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_FLOAT64: printf_("FLOAT64 "), printf_("%f\n", field->GetFLOAT64Value()); break;
//             case FIT_BASE_TYPE_STRING: printf_("STRING "), printf_("%ls\n", field->GetSTRINGValue().c_str()); break;
//             default: break;
//             }
// #else
//             switch(field->GetType()) {
//             // Get float 64 values for numeric types to receive values that have
//             // their scale and offset properly applied.
//             case FIT_BASE_TYPE_ENUM:
//             case FIT_BASE_TYPE_BYTE:
//             case FIT_BASE_TYPE_SINT8:
//             case FIT_BASE_TYPE_UINT8:
//             case FIT_BASE_TYPE_SINT16:
//             case FIT_BASE_TYPE_UINT16:
//             case FIT_BASE_TYPE_SINT32:
//             case FIT_BASE_TYPE_UINT32:
//             case FIT_BASE_TYPE_SINT64:
//             case FIT_BASE_TYPE_UINT64:
//             case FIT_BASE_TYPE_UINT8Z:
//             case FIT_BASE_TYPE_UINT16Z:
//             case FIT_BASE_TYPE_UINT32Z:
//             case FIT_BASE_TYPE_UINT64Z:
//             case FIT_BASE_TYPE_FLOAT32:
//             case FIT_BASE_TYPE_FLOAT64:
//                 printf_("%f %s\n",
//                     field->GetFLOAT64Value() /*/ profileField->scale*/,
//                     profileField->units.c_str());
//                 Records.back().setField(profileField->name,
//                     field->GetFLOAT64Value() /*/ profileField->scale*/);
//                 break;
//             case FIT_BASE_TYPE_STRING:
//                 printf_("%ls\n", field->GetSTRINGValue().c_str());
//                 break;
//             default:
//                 break;
//             }
// #endif
//         }
//     }

//     void OnMesg(fit::RecordMesg& record) override {
//         printf_("%s", "Record:\n");
//         if(Records.empty())
//             Records.reserve(10000000);
//         // if(Records.size() > 100) return;
//         Records.emplace_back(Record{});

//         PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Altitude);
//         PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Cadence);
//         PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Distance);
//         PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::EnhancedAltitude);
//         PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::EnhancedSpeed);
//         PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Grade);
//         PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::HeartRate);
//         PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::PositionLat);
//         PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::PositionLong);
//         PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Speed);
//         PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Temperature);
//         PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Timestamp);

//                // PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::HeartRate);
//                // PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Cadence);
//                // PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Distance);
//                // PrintOverrideValues(record, fit::RecordMesg::FieldDefNum::Speed);
//     }

//     void OnDeveloperFieldDescription(const fit::DeveloperFieldDescription& desc) override {
//         printf_("%s", "New Developer Field Description\n");
//         printf_("   App Version: %d\n", desc.GetApplicationVersion());
//         printf_("   Field Number: %d\n", desc.GetFieldDefinitionNumber());
//     }
// };

// Fit::Fit(QObject* parent)
//     : QObject{parent} { }

// /*
// // Java class
// package org.qtproject.qt5;
// class TestClass {
//     static String fromNumber(int x) { ... }
//     static String[] stringArray(String s1, String s2) { ... }
// }
// // C++ code
// // The signature for the first function is "(I)Ljava/lang/String;"
// QJniObject stringNumber
//     = QJniObject::callStaticObjectMethod("org/qtproject/qt5/TestClass",
//         "fromNumber"
//         "(I)Ljava/lang/String;",
//         10);
// // the signature for the second function is "(Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;"
// QJniObject string1 = QJniObject::fromString("String1");
// QJniObject string2 = QJniObject::fromString("String2");
// QJniObject stringArray = QJniObject::callStaticObjectMethod("org/qtproject/qt5/TestClass",
//     "stringArray"
//     "(Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;" string1.object<jstring>(),
//     string2.object<jstring>());

//  */

// #include <QApplication>
// #include <qpa/qplatformnativeinterface.h>

// QJniObject getMainActivity() {
//     QPlatformNativeInterface* interface = QApplication::platformNativeInterface();
//     QJniObject activity = (jobject)interface->nativeResourceForIntegration("QtActivity");
//     if(!activity.isValid())
//         qDebug() << "CLASS NOT VALID!!!!!!!!";
//     else
//         qDebug() << "HORRAY!";
//     return activity;
// }

// static QString getRealPathFromUri(const QUrl& url) {
// #if 1
//     QJniEnvironment env;
//     jclass clazz = env.findClass("org/xrsoft/utils/FileUtils");
//     jmethodID methodId = env.findStaticMethod(clazz,
//         "getPath2",
//         "(Ljava/lang/String)Ljava/lang/String;");
//     if(methodId != nullptr) {
//         QJniObject str = QJniObject::callStaticObjectMethod(clazz, methodId,
//                                                                              // getMainActivity().object<jobject>(),
//             QJniObject::fromString(url.toString()).object<jstring>());
//         return str.toString();
//     }

//            // QJniObject stringNumber = QJniObject::callStaticObjectMethod(
//            //     "org/xrsoft/utils/QSharePathResolver",
//            //     "getRealPathFrom",
//            //     "(Ljava/lang/Object;Ljava/lang/String)Ljava/lang/String;",
//            //     getMainActivity().object<jobject>(),
//            //     QJniObject::fromString(url.toString()).object<jstring>());

//     return {};

// #else
//     QString path = "";
//     QFileInfo info = QFileInfo(url.toString());
//     if(info.isFile()) {
//         QString abs = QFileInfo(url.toString()).absoluteFilePath();
//         if(!abs.isEmpty() && abs != url.toString() && QFileInfo(abs).isFile())
//             return abs;
//     } else if(info.isDir()) {
//         QString abs = QFileInfo(url.toString()).absolutePath();
//         if(!abs.isEmpty() && abs != url.toString() && QFileInfo(abs).isDir())
//             return abs;
//     }
//     QString localfile = url.toLocalFile();
//     if((QFileInfo(localfile).isFile() || QFileInfo(localfile).isDir()) && localfile != url.toString())
//         return localfile;
// #ifdef Q_OS_ANDROID
//     QJniObject jUrl = QJniObject::fromString(url.toString());
//     QJniObject jContext = getMainActivity(); // QtAndroidPrivate::context();
//     QJniObject jContentResolver = jContext.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
//     QJniObject jUri = QJniObject::callStaticObjectMethod("android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;", jUrl.object<jstring>());
//     QJniObject jCursor = jContentResolver.callObjectMethod("query", "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;", jUri.object<jobject>(), nullptr, nullptr, nullptr, nullptr);
//     QJniObject jScheme = jUri.callObjectMethod("getScheme", "()Ljava/lang/String;");
//     QJniObject authority;
//     if(jScheme.isValid())
//         authority = jUri.callObjectMethod("getAuthority", "()Ljava/lang/String;");
//     if(authority.isValid() && authority.toString() == "com.android.externalstorage.documents") {
//         QJniObject jPath = jUri.callObjectMethod("getPath", "()Ljava/lang/String;");
//         path = jPath.toString();
//     } else if(jCursor.isValid() && jCursor.callMethod<jboolean>("moveToFirst")) {
//         QJniObject jColumnIndex = QJniObject::fromString("_data");
//         jint columnIndex = jCursor.callMethod<jint>("getColumnIndexOrThrow", "(Ljava/lang/String;)I", jColumnIndex.object<jstring>());
//         QJniObject jRealPath = jCursor.callObjectMethod("getString", "(I)Ljava/lang/String;", columnIndex);
//         path = jRealPath.toString();
//         if(authority.isValid() && authority.toString().startsWith("com.android.providers") && !url.toString().startsWith("content://media/external/")) {
//             QStringList list = path.split(":");
//             if(list.count() == 2) {
//                 QString type = list.at(0);
//                 QString id = list.at(1);
//                 if(type == "image")
//                     type = type + "s";
//                 if(type == "document" || type == "documents")
//                     type = "file";
//                 if(type == "msf")
//                     type = "downloads";
//                 if(QList<QString>({"images", "video", "audio"}).contains(type))
//                     type = type + "/media";
//                 path = "content://media/external/" + type;
//                 path = path + "/" + id;
//                 return getRealPathFromUri(path);
//             }
//         }
//     } else {
//         QJniObject jPath = jUri.callObjectMethod("getPath", "()Ljava/lang/String;");
//         path = jPath.toString();
//         qDebug() << QFile::exists(path) << path;
//     }

//     if(path.startsWith("primary:")) {
//         path = path.remove(0, QString("primary:").length());
//         path = "/sdcard/" + path;
//     } else if(path.startsWith("/document/primary:")) {
//         path = path.remove(0, QString("/document/primary:").length());
//         path = "/sdcard/" + path;
//     } else if(path.startsWith("/tree/primary:")) {
//         path = path.remove(0, QString("/tree/primary:").length());
//         path = "/sdcard/" + path;
//     } else if(path.startsWith("/storage/emulated/0/")) {
//         path = path.remove(0, QString("/storage/emulated/0/").length());
//         path = "/sdcard/" + path;
//     } else if(path.startsWith("/tree//")) {
//         path = path.remove(0, QString("/tree//").length());
//         path = "/" + path;
//     }
//     if(!QFileInfo(path).isFile() && !QFileInfo(path).isDir() && !path.startsWith("/data"))
//         return url.toString();
//     return path;
// #else
//     return url.toString();
// #endif
// #endif
// }

// bool Fit::loadFile(const QString& filePath) {
//     QElapsedTimer timer;
//     timer.start();

//     fit::Decode decode;
//     // decode.SkipHeader();       // Use on streams with no header and footer (stream contains FIT defn and data messages only)
//     // decode.IncompleteStream(); // This suppresses exceptions with unexpected eof (also incorrect crc)
//     fit::MesgBroadcaster mesgBroadcaster;
//     Listener listener;

//     qInfo("FIT Decode Example Application\n");
//     qInfo() << filePath;
// #ifdef Q_OS_ANDROID
//     // auto argv = '/' + filePath.split("%3A%2F").back().replace("%2F", "/").toStdString();
//     // qInfo() << QDir{}.exists(filePath);
//     // qInfo() << QDir{}.exists(argv.c_str());

//            // QFileInfo fi{filePath};

//            // qCritical() << "absoluteFilePath" << fi.absoluteFilePath();
//            // qCritical() << "absolutePath" << fi.absolutePath();
//            // qCritical() << "baseName" << fi.baseName();
//            // qCritical() << "bundleName" << fi.bundleName();
//            // qCritical() << "canonicalFilePath" << fi.canonicalFilePath();
//            // qCritical() << "canonicalPath" << fi.canonicalPath();
//            // qCritical() << "completeBaseName" << fi.completeBaseName();
//            // qCritical() << "completeSuffix" << fi.completeSuffix();
//            // qCritical() << "fileName" << fi.fileName();
//            // qCritical() << "filePath" << fi.filePath();
//            // qCritical() << "group" << fi.group();
//            // qCritical() << "junctionTarget" << fi.junctionTarget();
//            // qCritical() << "owner" << fi.owner();
//            // qCritical() << "path" << fi.path();
//            // qCritical() << "readSymLink" << fi.readSymLink();
//            // qCritical() << "suffix" << fi.suffix();
//            // qCritical() << "symLinkTarget" << fi.symLinkTarget();

//            // qCritical() << "symLinkTarget" << fi.dir();
//            // qCritical() << "symLinkTarget" << fi.absoluteDir();

//            // qCritical() << "filesystemAbsoluteFilePath" << fi.filesystemAbsoluteFilePath().c_str();
//            // qCritical() << "filesystemAbsolutePath" << fi.filesystemAbsolutePath().c_str();
//            // qCritical() << "filesystemCanonicalFilePath" << fi.filesystemCanonicalFilePath().c_str();
//            // qCritical() << "filesystemCanonicalPath" << fi.filesystemCanonicalPath().c_str();
//            // qCritical() << "filesystemFilePath" << fi.filesystemFilePath().c_str();
//            // qCritical() << "filesystemJunctionTarget" << fi.filesystemJunctionTarget().c_str();
//            // qCritical() << "filesystemPath" << fi.filesystemPath().c_str();
//            // qCritical() << "filesystemReadSymLink" << fi.filesystemReadSymLink().c_str();
//            // qCritical() << "filesystemSymLinkTarget" << fi.filesystemSymLinkTarget().c_str();

//            // QJniObject uri = QJniObject::callStaticObjectMethod(
//            //     "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
//            //     QJniObject::fromString(filePath).object<jstring>());

//            // QString filename = QJniObject::callStaticObjectMethod(
//            //     "br/com/myjavapackage/PathUtil", "getFileName",
//            //     "(Landroid/net/Uri;Landroid/content/Context;)Ljava/lang/String;",
//            //     uri.object() /*, QtAndroid::androidContext().object()*/)
//            //                        .toString();

//     QByteArray data;
//     if(QFile fff{filePath}; fff.open(QFile::ReadOnly))
//         data = fff.readAll();

//     qWarning() << "data.size" << data.size();

//        // auto argv = getRealPathFromUri(filePath).toStdString();
// #else
//     auto argv = filePath.toStdString();
// #endif

//     struct OneShotReadBuf : public std::streambuf {
//         OneShotReadBuf(QByteArray& data) {
//             setg(data.data(), data.data(), data.data() + data.size()-1);
//         }
//     } osrb{data};

//     std::istream file{&osrb};
//     // std::istringstream file{data.data(), data.size()};

//            // file.open(argv, std::ios::in | std::ios::binary);

//            // if(!file.is_open()) {
//            //     qWarning("Error opening file %s\n", argv.c_str());
//            //     int error = errno;
//            //     const char* errorMessage = strerror(error);
//            //     qWarning() << "Error opening file: " << errorMessage;
//            // }

//     if(!decode.CheckIntegrity(file))
//         qWarning("FIT file integrity failed.\nAttempting to decode...\n");

//            // mesgBroadcaster.AddListener(static_cast<fit::FileIdMesgListener&>(listener));
//     mesgBroadcaster.AddListener(static_cast<fit::UserProfileMesgListener&>(listener));
//     // mesgBroadcaster.AddListener(static_cast<fit::MonitoringMesgListener&>(listener));
//     // mesgBroadcaster.AddListener(static_cast<fit::DeviceInfoMesgListener&>(listener));
//     mesgBroadcaster.AddListener(static_cast<fit::RecordMesgListener&>(listener));
//     // mesgBroadcaster.AddListener(static_cast<fit::MesgListener&>(listener));

//     try {
//         decode.Read(&file, &mesgBroadcaster, &mesgBroadcaster, &listener);
//     } catch(const fit::RuntimeException& e) {
//         qCritical("Exception decoding file: %s\n", e.what());
//         return -1;
//     }

//     Records.shrink_to_fit();

//     str.resize(100000);
//     qDebug() << str.toLocal8Bit().data();

//            // qInfo("Decoded FIT file %s.\n", argv.c_str());

//     qCritical() << "elapsed" << timer.elapsed() << "ms";
//     // exit(-99);
//     // str.resize(1000);
//     setText(str);

//     return 0;
// }
