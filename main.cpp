#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/config/AWSProfileConfigLoader.h>
#include <aws/core/platform/Environment.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <fstream>
#include <unistd.h>
#include <aws/transfer/TransferManager.h>
#include <aws/transfer/TransferHandle.h>
#include <aws/transfer/Transfer_EXPORTS.h>


Aws::Vector<Aws::String> parse_request(Aws::String request)
{
    Aws::Vector<Aws::String> parts = Aws::Utils::StringUtils::Split(request, '?');
    Aws::String header = parts.front();
    Aws::Vector<Aws::String> keys = Aws::Utils::StringUtils::Split(parts.back(), '&');

    Aws::String mp3_desc;
    Aws::String mp3_path;
    Aws::String mp3_name;
    Aws::Vector<Aws::String> mp3_stuff;

    Aws::String wav_desc;
    Aws::String wav_path;
    Aws::String wav_name;
    Aws::Vector<Aws::String> wav_stuff;

    Aws::Vector<Aws::String> useful_data;
    useful_data.push_back(parts.front());
    if (keys.size() < 2)
    {
        wav_desc = keys.front();
        wav_stuff = Aws::Utils::StringUtils::Split(wav_desc, '=');
        wav_path = wav_stuff.back();
        wav_stuff = Aws::Utils::StringUtils::Split(wav_path, '/');
        wav_name = wav_stuff.back();

        useful_data.push_back(wav_path);
        useful_data.push_back(wav_name);
    }else{
        mp3_desc = keys.front();
        mp3_stuff = Aws::Utils::StringUtils::Split(mp3_desc, '=');
        mp3_path = mp3_stuff.back();
        mp3_stuff = Aws::Utils::StringUtils::Split(mp3_path, '/');
        mp3_name = mp3_stuff.back();

        wav_desc = keys.back();
        wav_stuff = Aws::Utils::StringUtils::Split(wav_desc, '=');
        wav_path = wav_stuff.back();
        wav_stuff = Aws::Utils::StringUtils::Split(wav_path, '/');
        wav_name = wav_stuff.back();

        useful_data.push_back(mp3_path);
        useful_data.push_back(mp3_name);
        useful_data.push_back(wav_path);
        useful_data.push_back(wav_name);
    }
    /*
    for(int i=0; i< useful_data.size(); i++)
    {
        std::cout << useful_data.at(i) << std::endl;
    }
    */
    return useful_data;
}
struct wav_header_t
{
    char chunkID[4];
    unsigned long chunkSize;
    char format[4];
    char subchunk1ID[4];
    unsigned long subchunk1Size;
    unsigned short audioFormat;
    unsigned short numChannels;
    unsigned long sampleRate;
    unsigned long byteRate;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
};

struct json_output
{};

void show_info_like_json()
{}

void get_file_info(const char* fileName)
{
    FILE *audio_file = fopen(fileName, "rb");

    //Read WAV header
    wav_header_t header;
    fread(&header, sizeof(header), 1, audio_file);
    unsigned short channels     = header.numChannels;
    unsigned long sample_rate   = header.sampleRate;
    fclose(audio_file);
}

int download_file(const Aws::String &bucket_name, const Aws::String &object_name, const Aws::String &file_name) {

    Aws::Auth::AWSCredentials credentials;
    credentials.SetAWSAccessKeyId("AKIAIKMJ3L6NWJCU3VHQ");
    credentials.SetAWSSecretKey("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");

    const char* S3_CLIENT_TAG = "s3_client";
    Aws::Client::ClientConfiguration client_config;
    client_config.region = Aws::Region::US_WEST_2;
    client_config.scheme = Aws::Http::Scheme::HTTP;
    client_config.connectTimeoutMs = 60000;
    client_config.requestTimeoutMs = 600000;
    client_config.maxConnections = 500;
    //config.endpointOverride = "localhost:8089";
    auto s3_client = Aws::MakeShared<Aws::S3::S3Client>(S3_CLIENT_TAG, credentials, client_config);

    Aws::Utils::Threading::DefaultExecutor executor;
    Aws::Transfer::TransferManagerConfiguration transfer_manager_config(&executor);
    transfer_manager_config.s3Client = s3_client;
    transfer_manager_config.transferExecutor = &executor;
    //transfer_manager_config.errorCallback =
    auto transfer_manager = Aws::Transfer::TransferManager::Create(transfer_manager_config);

    auto transfer_handle = transfer_manager->DownloadFile(bucket_name, object_name, file_name);

    transfer_handle->WaitUntilFinished();
    bool success = transfer_handle->GetStatus() == Aws::Transfer::TransferStatus::COMPLETED;
    printf("TRANSFER HANDLE STATUS: %d", (int)transfer_handle->GetStatus());
    std::cout << success << std::endl;

    return 0;
}

int get_port_value(int argc, char** argv)
{
    std::string options = ":p:";
    const char *opts = options.c_str();
    int port_value = -1;
    int opt = getopt( argc, argv, opts );
    while( opt != -1 ) {
        switch(opt) {
            case 'p':
                port_value = atoi(optarg);
                break;
            default:
                break;
        }
        opt = getopt( argc, argv, opts );
    }
    return port_value;
}

int main(int argc, char** argv)
{

    int mode = 0;
    if (argc < 2)
    {
        std::cout << std::endl <<
                  "Not enough arguments\n" << std::endl;
        exit(1);
    }

    Aws::Vector <Aws::String> val = parse_request(argv[1]);
    int port_value = get_port_value(argc, argv);
    if(port_value != -1)
    {
        //set_new_port_value(port_value);
    }

    Aws::String bucket_name = "bucket-shmucket-right-here";
    Aws::String object_name;
    Aws::String file_name;
    Aws::String result_name;
    if(val.front() == "/wav-info")
    {
        object_name     = val.at(1);
        file_name       = val.at(2);
    }else{
        if(val.front() == "/mp3-to-wav")
        {
            object_name     = val.at(1);
            file_name       = val.at(2);
            result_name     = val.back();
            mode = 1;
        }else{exit(1);}
    }

    Aws::SDKOptions options;

    Aws::InitAPI(options);
    {
        download_file(bucket_name, object_name, file_name);
    }

    Aws::ShutdownAPI(options);

    std::string sox_command;
    std::string dir;

    if(mode)
    {
        sox_command.append("sox ");
        sox_command.append(file_name.c_str());
        sox_command.append(" -r 8000 ");
        sox_command.append(result_name.c_str());

        const char* new_temp_mode_one = sox_command.c_str();
        system(new_temp_mode_one);

        sox_command.clear();
        sox_command.append("soxi ");
        sox_command.append(file_name.c_str());
        std::cout << sox_command << std::endl;
        const char* new_temp_mode_zero= sox_command.c_str();
        system(new_temp_mode_zero);
    }
    else{
        sox_command.append("soxi ");
        sox_command.append(file_name.c_str());
        const char* new_temp_mode_zero= sox_command.c_str();
        system(new_temp_mode_zero);
    }

}
