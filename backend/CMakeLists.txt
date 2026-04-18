cmake_minimum_required(VERSION 3.5)
project(YearReporter CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} main.cc)

find_package(Drogon CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)

message(STATUS "use c++20")

aux_source_directory(controllers CTL_SRC)
aux_source_directory(controllers/Attendance CTL_ATTENDANCE_SRC)
aux_source_directory(controllers/Chronotype CTL_CHRONOTYPE_SRC)
aux_source_directory(controllers/Conspects CTL_CONSPECTS_SRC)
aux_source_directory(controllers/FirstVideoStarter CTL_FIRST_VIDEO_STARTER_SRC)
aux_source_directory(controllers/Media CTL_MEDIA_SRC)
aux_source_directory(controllers/Points CTL_POINTS_SRC)
aux_source_directory(controllers/Progress CTL_PROGRESS_SRC)
aux_source_directory(controllers/RareLessons CTL_RARE_LESSONS_SRC)
aux_source_directory(controllers/RarestVisitedLesson CTL_RAREST_VISITED_LESSON_SRC)
aux_source_directory(controllers/SolvedLessons CTL_SOLVED_LESSONS_SRC)
aux_source_directory(controllers/StudyRecommendations CTL_STUDY_RECOMMENDATIONS_SRC)
aux_source_directory(controllers/VideoAttention CTL_VIDEO_ATTENTION_SRC)
aux_source_directory(controllers/Videos CTL_VIDEOS_SRC)
aux_source_directory(services SERVICE_SRC)
aux_source_directory(constants CONSTANTS_SRC)

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                   ${CMAKE_CURRENT_SOURCE_DIR}/controllers
                                   ${CMAKE_CURRENT_SOURCE_DIR}/services
                                   ${CMAKE_CURRENT_SOURCE_DIR}/constants)

target_sources(${PROJECT_NAME}
               PRIVATE
               ${CTL_SRC}
               ${CTL_ATTENDANCE_SRC}
               ${CTL_CHRONOTYPE_SRC}
               ${CTL_CONSPECTS_SRC}
               ${CTL_FIRST_VIDEO_STARTER_SRC}
               ${CTL_MEDIA_SRC}
               ${CTL_POINTS_SRC}
               ${CTL_PROGRESS_SRC}
               ${CTL_RARE_LESSONS_SRC}
               ${CTL_RAREST_VISITED_LESSON_SRC}
               ${CTL_SOLVED_LESSONS_SRC}
               ${CTL_STUDY_RECOMMENDATIONS_SRC}
               ${CTL_VIDEO_ATTENTION_SRC}
               ${CTL_VIDEOS_SRC}
               ${SERVICE_SRC}
               ${CONSTANTS_SRC})
