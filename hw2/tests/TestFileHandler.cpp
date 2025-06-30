#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "FileHandler.h"


class TestFileHandler : public ::testing::Test {
protected:
    void SetUp() override {
        temp_file_path = std::filesystem::temp_directory_path() / (
                             "test_" + std::to_string(std::time(nullptr)) + ".txt");
    }

    void TearDown() override {
        if (std::filesystem::exists(temp_file_path)) {
            std::filesystem::remove(temp_file_path);
        }
    }

    void createTestFile(const std::vector<std::string_view> &lines) const {
        std::ofstream out(temp_file_path);
        for (const auto &line: lines) {
            out << line << '\n';
        }
        out.close();
    }

    std::filesystem::path temp_file_path;
};

// Тест открытия файла, которого не существует
TEST(FileHandlerTest, OpenNonExistentFile) {
    EXPECT_THROW({
                 FileHandler file("not_exist.txt", FileHandler::FileMode::Read);
                 }, FileHandler::FileNotFoundException);
}

// Тест успешного открытия файла для записи
TEST_F(TestFileHandler, OpenFileForWriting) {
    EXPECT_NO_THROW({
        FileHandler file(temp_file_path.string(), FileHandler::FileMode::Write);
        });
}

// Тест записи одной строки
TEST_F(TestFileHandler, WriteSingleLine) {
    FileHandler file(temp_file_path.string(), FileHandler::FileMode::Write);
    ASSERT_NO_THROW(file.writeLine("line"));

    std::ifstream in(temp_file_path);
    std::string line;
    ASSERT_TRUE(std::getline(in, line));
    EXPECT_EQ(line, "line");
    EXPECT_FALSE(std::getline(in, line));
}

// Тест записи нескольких строк
TEST_F(TestFileHandler, WriteMultipleLines) {
    FileHandler file(temp_file_path.string(), FileHandler::FileMode::Write);
    ASSERT_NO_THROW(file.writeLine("line1"));
    ASSERT_NO_THROW(file.writeLine("line2"));

    std::ifstream in(temp_file_path);
    std::string line;
    ASSERT_TRUE(std::getline(in, line));
    EXPECT_EQ(line, "line1");
    ASSERT_TRUE(std::getline(in, line));
    EXPECT_EQ(line, "line2");
    EXPECT_FALSE(std::getline(in, line));
}

// Тест чтения из пустого файла
TEST_F(TestFileHandler, ReadEmptyFile) {
    createTestFile({});
    FileHandler file(temp_file_path.string(), FileHandler::FileMode::Read);
    std::string line;
    EXPECT_FALSE(file.readLine().has_value());
}

// Тест чтения нескольких строк
TEST_F(TestFileHandler, ReadMultipleLines) {
    std::vector<std::string_view> lines = {"line1", "line2"};
    createTestFile(lines);
    FileHandler file(temp_file_path.string(), FileHandler::FileMode::Read);
    std::string line;

    ASSERT_EQ(lines[0], file.readLine());
    ASSERT_EQ(lines[1], file.readLine());
    EXPECT_FALSE(file.readLine().has_value());
}

// Тест попытки записи в файл, открытый для чтения
TEST_F(TestFileHandler, WriteToReadOnlyFile) {
    createTestFile({"..."});
    FileHandler file(temp_file_path.string(), FileHandler::FileMode::Read);
    EXPECT_THROW(file.writeLine("..."), FileHandler::FileWriteException);
}
